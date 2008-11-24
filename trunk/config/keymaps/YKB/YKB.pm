# Copyright (C) Andrew Suffield <asuffield@debian.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

package YKB::YKB;

use strict;
use 5.6.0;
use warnings;

use Carp;
use YKB::YKM;
use IO::File;
use Storable qw/dclone/;
use YKB::YKBGrammar;
use File::Spec;

sub new
  {
    my $class = shift;
    my $keymap = shift;
    my $option = shift;

    my $self =
      {
       keymap => $keymap,
       option => $option,
      };

    bless $self, $class;
    return $self;
  }

sub load
  {
    my $class = shift;
    my $file = shift;
    my $strict = shift;

    my $self =
      {
       file => $file,
      };

    bless $self, $class;
    return $self->_load($strict);
  }

my $parser = new YKB::YKBGrammar;

sub _parse_file
  {
    my $filename = shift;
    my $fh = new IO::File $filename, "r";
    unless ($fh)
      {
        print STDERR "Failed to open $filename: $!\n";
        return undef;
      }

    local $/ = undef;
    my $data = <$fh>;
    $fh->close;

    $::YKB_file = $filename;
    my $parsed = $parser->file($data);

    print STDERR "Failed to parse $filename\n" unless defined $parsed;

    return $parsed;
  }

sub expand_block_include
  {
    my $self = shift;
    my $block = shift;
    my $blocks = shift;

    my $errors = 0;

    my %included = ($block->{type} => {$block->{name} => 1});

    my $statements = $block->{statements};

    for (my $i = 0; $i < @$statements; $i++)
      {
        # We need this in here, because we're going to use redo and it
        # skips the condition
        last if $i >= @$statements;

        my $statement = $statements->[$i];
        # Splice out comments
        unless (ref $statement)
          {
            splice @$statements, $i, 1;
            redo;
          }

        if ($statement->{type} eq 'option')
          {
            my $statements = $statement->{statements};
            $errors += $self->expand_block_include($_, $blocks) foreach grep {exists $_->{statements}} @$statements;
            next;
          }

        next unless $statement->{include};

        my $file = $statement->{file} || die "invalid statement";
        my $line = $statement->{line} || 0;
        my $type = $statement->{type};
        my $name = $statement->{name};
        die "$file:$line: undefined include type" unless defined $type;
        die "$file:$line: undefined include name" unless defined $type;

        unless (exists $blocks->{$type}{$name})
          {
            print STDERR "$file:$line: unknown $type block '$name'\n";
            $errors++;

            # Splice it out
            splice @$statements, $i, 1;
            redo;
          }

        if ($included{$type}{$name})
          {
            # Splice it out
            splice @$statements, $i, 1;
            redo;
          }

        $included{$type}{$name}++;

        my $target = $blocks->{$type}{$name};
        my $included = $target->{statements};

        # Splice the included statements in place of the include statement
        splice @$statements, $i, 1, @$included;
        redo;
      }

    return $errors;
  }

sub parse_modifier
  {
    my $self = shift;
    my $mask = shift;

    my @value;

    foreach my $modifier (@$mask)
      {
        my $file = $modifier->{file};
        my $line = $modifier->{line};
        if (exists $modifier->{index})
          {
            push @value, {value => 1 << $modifier->{index}};
          }
        elsif (exists $modifier->{name})
          {
            my $name = $modifier->{name};
            push @value, {name => $name};
          }
        else
          {
            die "$file:$line: invalid modifier mask";
          }
      }

    return \@value;
  }

sub parse_sequence
  {
    my $self = shift;
    my $seq = shift;
    my $file = shift;
    my $line = shift;

    my @result;

    foreach my $member (@$seq)
      {
        if (exists $member->{keycode})
          {
            my $modifiers = exists $member->{modifiers} ? $self->parse_modifier($member->{modifiers}) : [];
            my $mask = exists $member->{mask} ? $self->parse_modifier($member->{mask}) : [{value => (1 << 16) - 1}];

            if (not defined $member->{direction})
              {
                push @result, {direction => 1, keycode => $member->{keycode}, modifiers => $modifiers, mask => $mask};
              }
            elsif ($member->{direction} eq '^')
              {
                push @result, {direction => 0, keycode => $member->{keycode}, modifiers => $modifiers, mask => $mask};
              }
            elsif ($member->{direction} eq '*')
              {
                push @result, {direction => 1, keycode => $member->{keycode}, modifiers => $modifiers, mask => $mask};
                push @result, {direction => 0, keycode => $member->{keycode}, modifiers => dclone($modifiers), mask => dclone($mask)};
              }
          }
        elsif (exists $member->{action})
          {
            my $arg = $member->{arg};
            if (defined $arg and ref $arg eq 'HASH')
              {
                if (exists $arg->{modifiers})
                  {
                    $arg->{modifiers} = $self->parse_modifier($arg->{modifiers});
                    $arg->{not} = defined $arg->{not};
                  }
              }
            push @result, {action => $member->{action}, arg => $member->{arg}};
          }
        else
          {
            die "$file:$line: invalid seq member";
          }
      }

    return \@result;
  }

sub parse_keymap
  {
    my $self = shift;
    my $block = shift;
    my $parent = shift;

    my $warnings = 0;
    my $errors = 0;

    my $keymap = {name => $block->{name},
                  keycode => {},
                  modifier => {},
                  'keycode name' => [],
                  'modifier name' => [],
                  seq => [],
                  flush => 0,
                  file => $block->{file},
                  line => $block->{line},
                 };

    my $statements = $block->{statements};

    my @options;

    foreach my $statement (@$statements)
      {
        my $file = $statement->{file} || die "invalid statement";
        my $line = $statement->{line} || 0;

        if ($statement->{type} eq 'flush')
          {
            $keymap->{keycode} = {};
            $keymap->{modifier} = {};
            $keymap->{'keycode name'} = [];
            $keymap->{'modifier name'} = [];
            $keymap->{seq} = [];
            $keymap->{flush} = 1;
          }
        elsif ($statement->{type} eq 'keycode')
          {
            my $name = $statement->{name};

            if (exists $keymap->{keycode}{$name})
              {
                my $oldfile = $keymap->{keycode}{$name}{file};
                my $oldline = $keymap->{keycode}{$name}{line};
                print STDERR "$file:$line: keycode '$name' redefined\n";
                print STDERR "$oldfile:$oldline: (previous definition here)\n";
                $warnings++;
              }

            if (exists $statement->{value})
              {
                my $value = $statement->{value};
                $keymap->{keycode}{$name} = {value => $value,
                                             file => $file,
                                             line => $line,
                                            };
              }
            elsif (exists $statement->{alias})
              {
                my $alias = $statement->{alias};
                my $modifiers = exists $statement->{modifiers} ? $self->parse_modifier($statement->{modifiers}) : [];
                my $mask = exists $statement->{mask} ? $self->parse_modifier($statement->{mask}) : [{value => (1 << 16) - 1}];

                $keymap->{keycode}{$name} = {alias => $alias,
                                             modifiers => $modifiers,
                                             mask => $mask,
                                             file => $file,
                                             line => $line,
                                            };
              }
            else
              {
                die "$file:$line: incomplete keycode statement";
              }
          }
        elsif ($statement->{type} eq 'modifier')
          {
            my $name = $statement->{name};
            my $mask = $statement->{value};

            if (exists $keymap->{modifier}{$name})
              {
                my $oldfile = $keymap->{modifier}{$name}{file};
                my $oldline = $keymap->{modifier}{$name}{line};
                print STDERR "$file:$line: modifier '$name' redefined\n";
                print STDERR "$oldfile:$oldline: (previous definition here)\n";
                $warnings++;
              }

            my $value = $self->parse_modifier($mask);
            unless (defined $value)
              {
                $errors++;
                next;
              }

            $keymap->{modifier}{$name} = {value => $value,
                                          file => $file,
                                          line => $line,
                                         };
          }
        elsif ($statement->{type} eq 'keycode name')
          {
            my $name = $statement->{name};
            my $desc = $statement->{desc};
            my $modifiers = exists $statement->{modifiers} ? $self->parse_modifier($statement->{modifiers}) : [];
            my $mask = exists $statement->{mask} ? $self->parse_modifier($statement->{mask}) : [{value => (1 << 16) - 1}];

            push @{$keymap->{'keycode name'}}, {keycode => $name,
                                                name => $desc,
                                                modifiers => $modifiers,
                                                mask => $mask,
                                                file => $file,
                                                line => $line,
                                               };
          }
        elsif ($statement->{type} eq 'modifier name')
          {
            my $modifiers = $statement->{modifiers};
            my $mask = $statement->{mask};
            my $desc = $statement->{desc};

            # If no mask was specified, set the mask to be the same as
            # the modifiers - "anything with precisely these set". This
            # is the "natural" behaviour.
            $mask = $modifiers unless defined $mask;

            my $modifiers_value = $self->parse_modifier($modifiers, $keymap);
            unless (defined $modifiers_value)
              {
                $errors++;
                next;
              }

            my $mask_value = $self->parse_modifier($mask, $keymap);
            unless (defined $mask_value)
              {
                $errors++;
                next;
              }

            push @{$keymap->{'modifier name'}}, {modifiers => $modifiers_value,
                                                 mask => $mask_value,
                                                 name => $desc,
                                                 file => $file,
                                                 line => $line,
                                                };
          }
        elsif ($statement->{type} eq 'seq')
          {
            my $seq = $self->parse_sequence($statement->{sequence}, $keymap, $statement->{file}, $statement->{line});
            unless (defined $seq)
              {
                $errors++;
                next;
              }

            push @{$keymap->{seq}}, $seq;
          }
        elsif ($statement->{type} eq 'key')
          {
            my $keycode = $statement->{keycode};
            my $args = $statement->{args};

            if (scalar @$args > 4)
              {
                print STDERR "$file:$line: too many arguments in key statement; ignoring the extras\n";
                $warnings++;
              }

            # We're only interested in modifiers shift and altgr
            my $mask = [{name => 'shift'}, {name => 'altgr'}];

            # A key statement is a simple 2x2 matrix:
            #
            #           no shift  |  shift
            # no altgr     1      |    2
            # altgr        3      |    4
            #
            # key xxx = 1, 2, 3, 4;

            # If defined, an argument flushes whatever that key previously did
            # If zero-length, no new action is inserted

            # Example: key xxx = undef, undef, "", "x";

            # This does not affect the non-altgr bindings, flushes the
            # altgr-only case, and sets shift-altgr to "x"

            # The string is also inserted as the key name

            if (defined $args->[0])
              {
                my $arg = $args->[0];
                my $modifiers = [];
                push @{$keymap->{seq}}, [{direction => 1, keycode => $keycode,
                                          modifiers => dclone($modifiers), mask => dclone($mask)},
                                         {action => "clear"},
                                         length $arg ? ({action => "string", arg => $arg}) : ()];
                push @{$keymap->{'keycode name'}}, {keycode => $keycode, name => $arg,
                                                    modifiers => dclone($modifiers), mask => dclone($mask),
                                                    file => $file, line => $line};
              }
            if (defined $args->[1])
              {
                my $arg = $args->[1];
                my $modifiers = [{name => 'shift'}];
                push @{$keymap->{seq}}, [{direction => 1, keycode => $keycode,
                                          modifiers => dclone($modifiers), mask => dclone($mask)},
                                         {action => "clear"},
                                         length $arg ? ({action => "string", arg => $arg}) : ()];
                push @{$keymap->{'keycode name'}}, {keycode => $keycode, name => $arg,
                                                    modifiers => dclone($modifiers), mask => dclone($mask),
                                                    file => $file, line => $line};
              }
            if (defined $args->[2])
              {
                my $arg = $args->[2];
                my $modifiers = [{name => 'altgr'}];
                push @{$keymap->{seq}}, [{direction => 1, keycode => $keycode,
                                          modifiers => dclone($modifiers), mask => dclone($mask)},
                                         {action => "clear"},
                                         length $arg ? ({action => "string", arg => $arg}) : ()];
                push @{$keymap->{'keycode name'}}, {keycode => $keycode, name => $arg,
                                                    modifiers => dclone($modifiers), mask => dclone($mask),
                                                    file => $file, line => $line};
              }
            if (defined $args->[3])
              {
                my $arg = $args->[3];
                my $modifiers = [{name => 'shift'}, {name => 'altgr'}];
                push @{$keymap->{seq}}, [{direction => 1, keycode => $keycode,
                                          modifiers => dclone($modifiers), mask => dclone($mask)},
                                         {action => "clear"},
                                         length $arg ? ({action => "string", arg => $arg}) : ()];
                push @{$keymap->{'keycode name'}}, {keycode => $keycode, name => $arg,
                                                    modifiers => dclone($modifiers), mask => dclone($mask),
                                                    file => $file, line => $line};
              }
          }
        elsif ($statement->{type} eq 'option')
          {
            if ($parent)
              {
                $statement->{parent} = $parent . "::" . $keymap->{name};
              }
            else
              {
                $statement->{parent} = $keymap->{name};
              }
            push @options, $statement;
          }
        else
          {
            die "$file:$line: unrecognised statement type $statement->{type}";
          }
      }

    return ($keymap, $errors, $warnings, \@options);
  }

sub _load
  {
    my $self = shift;
    my $strict = shift;

    my (undef, $base_path, undef) = File::Spec->splitpath($self->{file});

    my $parsed = _parse_file($self->{file});
    return undef unless defined $parsed;

    my %loaded = ($self->{file} => 1);

    # Now, first we study the blocks, making a note of all the
    # keymaps, options, and named blocks

    my @keymaps;
    my @options;
    my @blocks;
    my $errors = 0;
    my $warnings = 0;

    my %blocks = (keymap => {},
                  keycode => {},
                  'keycode name' => {},
                  modifier => {},
                  'modifier name' => {},
                  seq => {},
                  key => {},
                  option => {},
                 );

    foreach my $block (@$parsed)
      {
        # Skip comments
        next unless ref $block;

        my $file = $block->{file} || die "invalid block";
        my $line = $block->{line} || die "invalid block";
        my $type = $block->{type};
        die "$file:$line: undefined block type" unless defined $type;

        if ($type eq 'include')
          {
            my $name = $block->{name};
            my $absname = File::Spec->rel2abs($name, $base_path);
            if ($loaded{$absname})
              {
                # Silently discard
                next;
              }
            unless (-f $absname)
              {
                print STDERR "$file:$line: can't find file '$name'\n";
                $errors++;
                next;
              }
            $loaded{$absname}++;
            my $included = _parse_file($absname);
            unless (defined $included)
              {
                $errors++;
                next;
              }
            push @$parsed, @$included;
            next;
          }

        die "$file:$line: unrecognised block type '$block->{type}'" unless exists $blocks{$type};

        my $name = $block->{name};

        if (defined $name)
          {
            # Duplicates are discarded out of hand
            if (exists $blocks{$type}{$name})
              {
                my $oldfile = $blocks{$type}{$name}{file};
                my $oldline = $blocks{$type}{$name}{line};
                print STDERR "$file:$line: $type block '$name' redefined\n";
                print STDERR "$oldfile:$oldline: (previously defined here)\n";
                $errors++;
                next;
              }

            $blocks{$type}{$name} = $block;
          }
        else
          {
            # Only keymaps can be anonymous
            if ($type ne 'keymap')
              {
                print STDERR "$file:$line: useless definition of an anonymous $type block\n";
                $errors++;
                next;
              }
          }

        # We skip abstract keymaps here - they're only for use in
        # 'include' statements, not for output.
        push @keymaps, $block if $type eq 'keymap' and not $block->{abstract};
        push @options, $block if $type eq 'option';
      }

    # Don't need this any more
    undef $parsed;

    $self->{loaded} = \%loaded;

    unless (scalar @keymaps)
      {
        print STDERR "$self->{file}:0: at least one keymap is required\n";
        return undef;
      }

    # Now we expand every block include directive, in a recursive
    # descent fashion (note that include statements which can't be
    # reached from a keymap are never expanded). We also eliminate
    # comments here.

    $errors += $self->expand_block_include($_, \%blocks) foreach @keymaps;

    # And the same for optional keymaps
    foreach my $option (@options)
      {
        my $statements = $option->{statements};
        # Any statement which itself has a 'statements' member is a keymap block; expand it
        $errors += $self->expand_block_include($_, \%blocks) foreach grep {exists $_->{statements}} @$statements;
      }

    # Don't need these any more
    %blocks = ();

    # Now we actually process the statements

    $self->{keymap} = {};
    $self->{anonymous_keymaps} = 0;

    foreach my $block (@keymaps)
      {
        my $file = $block->{file};
        my $line = $block->{line};
        my $name = $block->{name};
        if (defined $name and exists $self->{keymap}{$name})
          {
            my $oldfile = $self->{keymap}{$name}{file};
            my $oldline = $self->{keymap}{$name}{line};
            print STDERR "$file:$line: keymap '$name' redefined\n";
            print STDERR "$oldfile:$oldline: (previous definition here)\n";
            $errors++;
            next;
          }

        my ($keymap, $new_errors, $new_warnings, $new_options) = $self->parse_keymap($block);
        $errors += $new_errors;
        $warnings += $new_warnings;
        push @options, @$new_options;
        next unless defined $keymap;

        $self->{keymap}{$keymap->{name}} = $keymap;
      }

    $self->{option} = {};

    foreach my $option (@options)
      {
        my $name = $option->{name};
        if (exists $self->{option}{$name})
          {
            my $file = $option->{file};
            my $line = $option->{line};
            my $oldfile = $self->{option}{$name}{file};
            my $oldline = $self->{option}{$name}{line};
            print STDERR "$file:$line: option '$name' redefined\n";
            print STDERR "$oldfile:$oldline: (previous definition here)\n";
            $errors++;
            next;
          }

        my $keymaps = {};
        $self->{option}{$name} = {name => $name,
                                  parent => $option->{parent},
                                  default => "",
                                  keymap => $keymaps,
                                  flush => 0,
                                  file => $option->{file},
                                  line => $option->{line},
                                 };

        foreach my $block (@{$option->{statements}})
          {
            my $file = $block->{file};
            my $line = $block->{line};

            my $type = $block->{type};
            if ($type eq 'flush')
              {
                $keymaps = {};
                $self->{option}{$name}{flush} = 1;
                $self->{option}{$name}{keymap} = $keymaps;
              }
            elsif ($type eq 'keymap')
              {
                my $keymap_name = $block->{name};

                if (exists $keymaps->{$keymap_name})
                  {
                    my $oldfile = $keymaps->{$keymap_name}{file};
                    my $oldline = $keymaps->{$keymap_name}{line};
                    print STDERR "$file:$line: option keymap '$keymap_name' redefined\n";
                    print STDERR "$oldfile:$oldline: (previous definition here)\n";
                    $errors++;
                    next;
                  }

                if ($block->{default})
                  {
                    if ($self->{option}{$name}{default} and $self->{option}{$name}{default} ne $block->{default})
                      {
                        print STDERR "$file:$line: default keymap for option '$name' redefined (old value was '$self->{option}{$name}{default}')\n";
                        $errors++;
                      }
                    $self->{option}{$name}{default} = $keymap_name;
                  }

                my $keymap;

                if (exists $block->{statements})
                  {
                    my ($new_errors, $new_warnings, $new_options);
                    ($keymap, $new_errors, $new_warnings, $new_options) = $self->parse_keymap($block, $name);
                    $errors += $new_errors;
                    $warnings += $new_warnings;
                    push @options, @$new_options;
                    next unless defined $keymap;
                  }
                else
                  {
                    unless (exists $self->{keymap}{$keymap_name})
                      {
                        print STDERR "$file:$line: undefined keymap '$keymap_name'\n";
                        $errors++;
                        next;
                      }

                    $keymap = $keymap_name;
                  }

                $keymaps->{$keymap_name} = $keymap;
              }
            else
              {
                die "$file:$line: unknown statement type '$type' in option block";
              }
          }
      }

    return undef if $errors;
    return undef if $warnings and $strict;
    return $self;
  }

sub print_nested
  {
    my $self = shift;
    my $fh = shift;
    my $nesting = shift;
    my $str = shift;
    print $fh "  " x $nesting;
    print $fh $str;
    print $fh "\n";
    return 1;
  }

sub format_modifier_value
  {
    my $self = shift;
    my $value = shift;

    return $value->{name} if exists $value->{name};

    my $v = $value->{value};

    # There should be precisely one bit set
    my @bits = grep {defined $_} map {($v & (1 << $_)) ? $_ : undef} 0..15;
    if (scalar @bits != 1)
      {
        confess "Invalid modifier value $v (should be a power of two)";
      }
    return $bits[0];
  }

sub format_modifier
  {
    my $self = shift;
    my $modifier = shift;

    return join('|', map {$self->format_modifier_value($_)} @$modifier);
  }

sub quote_string
  {
    my $str = shift;
    return 'undef' unless defined $str;
    $str =~ s/\\/\\\\/g;
    $str =~ s/"/\\"/g;
    return '"' . $str . '"';
  }

sub format_modifier_suffix
  {
    my $self = shift;
    my $modifiers = shift;
    my $mask = shift;
    my $default_match_all = shift;

    my $no_modifiers = scalar @{$modifiers} == 0;
    my $no_mask = scalar @{$mask} == 0;
    my $default_mask;

    if ($default_match_all)
      {
        $default_mask = $no_mask;
      }
    else
      {
        $default_mask = (scalar @{$mask} == 1)
          && exists $mask->[0]{value}
          && ($mask->[0]{value} == (1 << 16) - 1);
      }

    if ($no_modifiers and $no_mask and not $default_match_all)
      {
        return "[*]";
      }
    elsif ($no_modifiers and $default_mask)
      {
        return "";
      }
    elsif ($default_mask)
      {
        return "[" . $self->format_modifier($modifiers) . "]";
      }
    else
      {
        return "[" . $self->format_modifier($modifiers) . " / " .  $self->format_modifier($mask) . "]";
      }
  }

my %has_modifier_arg = map{$_=>1} qw/setModifiers maskModifiers toggleModifiers
                                     setStickyModifiers maskStickyModifiers toggleStickyModifiers/;
my %has_identifier_arg = map{$_=>1} qw/beginExtended unsetOption addKeymap removeKeymap/;
my %has_string_arg = map{$_=>1} qw/string event/;
my %has_no_arg = map{$_=>1} qw/abortExtended flushKeymap restoreState clear/;

sub format_seq_member
  {
    my $self = shift;
    my $member = shift;

    if (exists $member->{keycode})
      {
        my $dir_str = $member->{direction} ? "" : "^";

        return join('', $dir_str, $member->{keycode}, $self->format_modifier_suffix($member->{modifiers}, $member->{mask}));
      }
    else
      {
        if ($has_modifier_arg{$member->{action}})
          {
            return join('',
                        "$member->{action}(",
                        $member->{arg}{not} ? "!" : "",
                        $self->format_modifier($member->{arg}{modifiers}),
                        ")");
          }
        elsif ($has_identifier_arg{$member->{action}})
          {
            return "$member->{action}($member->{arg})";
          }
        elsif ($has_string_arg{$member->{action}})
          {
            return "$member->{action}(" . quote_string($member->{arg}) . ")";
          }
        elsif ($has_no_arg{$member->{action}})
          {
            return "$member->{action}()";
          }
        elsif ($member->{action} eq 'setOption')
          {
            return "setOption(" . join(', ', ($member->{arg}{option}, $member->{arg}{keymap})) . ")";
          }
        else
          {
            die "Unhandled action '$member->{action}'";
          }
      }
  }

sub format_seq
  {
    my $self = shift;
    my $seq = shift;

    my @members = map {$self->format_seq_member($_)} @$seq;
    for (my $i = 0; $i < $#members - 1; $i++)
      {
        # If the next member is precisely equal to this one, only with
        # a ^ prefix, then this is a down-up pair - so splice a *
        # prefix in place of the pair.
        if ('^' . $members[$i] eq $members[$i + 1])
          {
            splice @members, $i, 2, '*' . $members[$i];
          }
      }
    return join(' ', @members);
  }

sub output_keymap
  {
    my $self = shift;
    my $fh = shift;
    my $name = shift;
    my %keymap = %{shift()};
    my $option_children = shift;
    my $nested = shift || 0;
    my $parent = shift;
    my $default = shift;

    print $fh "\n" unless $nested;

    $self->print_nested($fh, $nested, ($default ? "default " : "") . "keymap $name");
    $self->print_nested($fh, $nested, "{");
    $self->print_nested($fh, $nested + 1, "flush;") if $keymap{flush};

    my $need_space = 0;

    if (scalar keys %{$keymap{keycode}} > 4)
      {
        $self->print_nested($fh, $nested + 1, "keycode");
        $self->print_nested($fh, $nested + 1, "{");
        foreach my $keycode (sort keys %{$keymap{keycode}})
          {
            if (exists $keymap{keycode}{$keycode}{value})
              {
                $self->print_nested($fh, $nested + 2, "$keycode = $keymap{keycode}{$keycode}{value};");
              }
            else
              {
                my $suffix = $self->format_modifier_suffix($keymap{keycode}{$keycode}{modifiers},
                                                           $keymap{keycode}{$keycode}{mask});
                $self->print_nested($fh, $nested + 2, "$keycode = $keymap{keycode}{$keycode}{alias}$suffix;");
              }
          }
        $self->print_nested($fh, $nested + 1, "}");
        $need_space = 1;
      }
    elsif (scalar keys %{$keymap{keycode}} > 0)
      {
        foreach my $keycode (sort keys %{$keymap{keycode}})
          {
            if (exists $keymap{keycode}{$keycode}{value})
              {
                $self->print_nested($fh, $nested + 1, "keycode $keycode = $keymap{keycode}{$keycode}{value};");
              }
            else
              {
                my $suffix = $self->format_modifier_suffix($keymap{keycode}{$keycode}{modifiers},
                                                           $keymap{keycode}{$keycode}{mask});
                $self->print_nested($fh, $nested + 1, "keycode $keycode = $keymap{keycode}{$keycode}{alias}$suffix;");
              }
          }
        $need_space = 1;
      }

    print $fh "\n" if $need_space;
    $need_space = 0;

    if (scalar keys %{$keymap{modifier}} > 4)
      {
        $self->print_nested($fh, $nested + 1, "modifier");
        $self->print_nested($fh, $nested + 1, "{");
        foreach my $modifier (sort keys %{$keymap{modifier}})
          {
            my $value = $self->format_modifier($keymap{modifier}{$modifier}{value});
            $self->print_nested($fh, $nested + 2, "$modifier = $value;");
          }
        $self->print_nested($fh, $nested + 1, "}");
        $need_space = 1;
      }
    elsif (scalar keys %{$keymap{modifier}} > 0)
      {
        foreach my $modifier (sort keys %{$keymap{modifier}})
          {
            my $value = $self->format_modifier($keymap{modifier}{$modifier}{value});
            $self->print_nested($fh, $nested + 1, "modifier $modifier = $value;");
          }
        $need_space = 1;
      }

    print $fh "\n" if $need_space;
    $need_space = 0;

    if (scalar @{$keymap{'keycode name'}} > 4)
      {
        $self->print_nested($fh, $nested + 1, "keycode name");
        $self->print_nested($fh, $nested + 1, "{");
        foreach my $kn (sort {$a->{keycode} cmp $b->{keycode}} @{$keymap{'keycode name'}})
          {
            my $keycode = $kn->{keycode};
            my $name = $kn->{name};
            my $suffix = $self->format_modifier_suffix($kn->{modifiers}, $kn->{mask});
            $self->print_nested($fh, $nested + 2, "$keycode$suffix = " . quote_string($name) . ";");
          }
        $self->print_nested($fh, $nested + 1, "}");
        $need_space = 1;
      }
    elsif (scalar @{$keymap{'keycode name'}} > 0)
      {
        foreach my $kn (sort {$a->{keycode} cmp $b->{keycode}} @{$keymap{'keycode name'}})
          {
            my $keycode = $kn->{keycode};
            my $name = $kn->{name};
            my $suffix = $self->format_modifier_suffix($kn->{modifiers}, $kn->{mask});
            $self->print_nested($fh, $nested + 1, "keycode name $keycode$suffix = " . quote_string($name) . ";");
          }
        $need_space = 1;
      }

    print $fh "\n" if $need_space;
    $need_space = 0;

    if (scalar @{$keymap{'modifier name'}} > 4)
      {
        $self->print_nested($fh, $nested + 1, "modifier name");
        $self->print_nested($fh, $nested + 1, "{");
        foreach my $modifier (@{$keymap{'modifier name'}})
          {
            my $mod_str = $self->format_modifier($modifier->{modifiers});
            my $mask_str = $self->format_modifier($modifier->{mask});
            $self->print_nested($fh, $nested + 2, "$mod_str/$mask_str = " . quote_string($modifier->{name}) . ";");
          }
        $self->print_nested($fh, $nested + 1, "}");
        $need_space = 1;
      }
    elsif (scalar @{$keymap{'modifier name'}} > 0)
      {
        foreach my $modifier (@{$keymap{'modifier name'}})
          {
            my $mod_str = $self->format_modifier($modifier->{modifiers});
            my $mask_str = $self->format_modifier($modifier->{mask});
            $self->print_nested($fh, $nested + 1, "modifier name $mod_str/$mask_str = " . quote_string($modifier->{name}) . ";");
          }
        $need_space = 1;
      }

    print $fh "\n" if $need_space;
    $need_space = 0;

    if (scalar @{$keymap{seq}} > 4)
      {
        $self->print_nested($fh, $nested + 1, "seq");
        $self->print_nested($fh, $nested + 1, "{");
        foreach my $seq (@{$keymap{seq}})
          {
            $self->print_nested($fh, $nested + 2, $self->format_seq($seq) . ";");
          }
        $self->print_nested($fh, $nested + 1, "}");
        $need_space = 1;
      }
    elsif (scalar @{$keymap{seq}} > 0)
      {
        foreach my $seq (@{$keymap{seq}})
          {
            $self->print_nested($fh, $nested + 1, "seq " . $self->format_seq($seq) . ";");
          }
        $need_space = 1;
      }

    my $full_name = $parent ? "${parent}::${name}" : $name;
    if (exists $option_children->{$full_name})
      {
        foreach my $option (@{$option_children->{$full_name}})
          {
            print $fh "\n" if $need_space;
            $self->output_option($fh, $option->{name}, $option, $option_children, $nested + 1);
            $need_space = 1;
          }
      }

    $self->print_nested($fh, $nested, "}");
  }

sub output_option
  {
    my $self = shift;
    my $fh = shift;
    my $name = shift;
    my $option = shift;
    my $option_children = shift;
    my $nested = shift || 0;

    $self->print_nested($fh, $nested, "option $name");
    $self->print_nested($fh, $nested, "{");
    $self->print_nested($fh, $nested + 1, "flush;") if $option->{flush};
    foreach my $keymap (keys %{$option->{keymap}})
      {
        if (ref $option->{keymap}{$keymap} eq 'HASH')
          {
            my $default = $option->{default} ? ($option->{default} eq $keymap) : undef;
            $self->output_keymap($fh, $keymap, $option->{keymap}{$keymap}, $option_children, $nested + 1, $name, $default);
            delete $option_children->{"${name}::${keymap}"};
          }
        else
          {
            $self->print_nested($fh, $nested + 1, "keymap $keymap;");
          }
      }
    $self->print_nested($fh, $nested, "}");
  }

sub output
  {
    my $self = shift;
    my $output = shift;

    my $fh = new IO::File $output, "w";
    unless ($fh)
      {
        print STDERR "Failed to open $output: $!\n";
        return 0;
      }

    print $fh "# Generated from a complied keymap by ykbcomp\n";

    my @root_options;
    my %option_children;
    foreach my $option (keys %{$self->{option}})
      {
        unless ($self->{option}{$option}->{parent})
          {
            push @root_options, $option;
            next;
          }
        push @{$option_children{$self->{option}{$option}->{parent}}}, $self->{option}{$option};
      }

    foreach (keys %{$self->{keymap}})
      {
        $self->output_keymap($fh, $_, $self->{keymap}{$_}, \%option_children, 0);
        delete $option_children{$_};
      }

    foreach (@root_options)
      {
        print $fh "\n";
        $self->output_option($fh, $_, $self->{option}{$_}, \%option_children, 0);
      }

    print STDERR "Discarding orphaned option children:\n" if scalar keys %option_children;
    foreach my $keymap (keys %option_children)
      {
        print STDERR "$keymap: " . join(', ', map {$_->{name}} @{$option_children{$keymap}}) . "\n";
      }

    return 1;
  }

sub loaded
  {
    my $self = shift;

    return keys %{$self->{loaded}};
  }

sub output_deps
  {
    my $self = shift;
    my $file = shift;
    my $target = shift;

    my @deps = $self->loaded();

    my $fh = new IO::File $file, "w";
    unless ($fh)
      {
        print STDERR "Failed to open $file: $!\n";
        return 0;
      }

    print $fh "$target: \\\n";
    print $fh join("\\\n", map {" $_ "} @deps);
    print $fh "\n\n";

    # This hack fixes the "deleted header file" problem
    print $fh "$_:\n" foreach @deps;

    return 1;
  }

sub ykm
  {
    my $self = shift;

    my $ykm = new YKB::YKM $self->{keymap}, $self->{option};
    return $ykm;
  }

1;
