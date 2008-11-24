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

package YCL::YCD;

use strict;
use 5.6.0;
use warnings;

use Carp;
use IO::File;

use overload '""' => \&name;

my %valid_type = map {$_=>1} qw/uint32 int32 object string ybool any .../;

my %normal = (uint32 => 'uint32',
              ybool => 'ybool',
              int32 => 'uint32',
              object => 'uint32',
              string => 'string',
              '...' => 'list',
             );

sub new
  {
    my $class = shift;
    my $class_name = shift;

    my $self =
      {
       class => $class_name,
       instance_method => {},
       class_method => {},
       property => {},
       super => [],
       source => [],
      };

    bless $self, $class;
    return $self;
  }

sub load
  {
    my $class = shift;
    my $file = shift;

    my $self =
      {
       class => undef,
       instance_method => {},
       class_method => {},
       property => {},
       super => [],
       source => [$file],
      };
    bless $self, $class;

    my $fh = new IO::File $file, "r";
    unless ($fh)
      {
        print STDERR "Couldn't open $file: $!\n";
        return undef;
      }

    my $errors = 0;

    while (<$fh>)
      {
        chomp;
        s/^\s+//;
        next if /^\#/;
        s/\s+$//;

        next unless length $_;

        my $line = $fh->input_line_number;

        unless (s/^(\S+)\s+//)
          {
            print STDERR "$file:$line: syntax error\n";
            $errors++;
            next;
          }

        my $keyword = $1;

        if ($keyword eq 'class')
          {
            if (defined $self->{class})
              {
                print STDERR "$file:$line: multiple class definitions in one file not allowed\n";
                $errors++;
                last;
              }
            $self->{class} = $_;
          }
        elsif ($keyword eq 'super')
          {
            $self->add_super($_);
          }
        elsif ($keyword eq 'property')
          {
            my ($name, $type) = split /\s+/;
            $self->add_property($name, $type);
          }
        elsif ($keyword eq 'property-hook')
          {
            my ($name, $function, $arg_convention) = split /\s+/;
            $self->add_property_hook($name, $function, $arg_convention);
          }
        elsif ($keyword eq 'class-method')
          {
            my ($method, $args, $result, $function, $has_client_arg, $arg_convention, $result_convention, $is_static) = split /\s+/;
            $args =~ s/^\(//;
            $args =~ s/\)$//;
            $result =~ s/^\(//;
            $result =~ s/\)$//;
            $self->add_class_method($method, [split /,/, $args], [split /,/, $result],
                                    $function, $has_client_arg, $arg_convention, $result_convention, $is_static);
          }
        elsif ($keyword eq 'instance-method')
          {
            my ($method, $args, $result, $function, $has_client_arg, $arg_convention, $result_convention, $is_static) = split /\s+/;
            $args =~ s/^\(//;
            $args =~ s/\)$//;
            $result =~ s/^\(//;
            $result =~ s/\)$//;
            $self->add_instance_method($method, [split /,/, $args], [split /,/, $result],
                                       $function, $has_client_arg, $arg_convention, $result_convention, $is_static);
          }
        else
          {
            print STDERR "$file:$line: unrecognised keyword '$keyword'\n";
            $errors++;
          }
      }

    return undef if $errors;
    return undef unless defined $self->{class};

    return $self;
  }

sub output
  {
    my $self = shift;
    my $file = shift;

    my $fh = new IO::File $file, "w";
    unless ($fh)
      {
        print STDERR "Couldn't create $file: $!\n";
        return 0;
      }

    print $fh "# Y Class Definition\n";
    print $fh "\n";
    print $fh "class $self->{class}\n";

    my %super;
    foreach my $super (@{$self->{super}})
      {
        next if $super{$super}++;
        print $fh "super $super\n";
      }

    foreach my $p (values %{$self->{property}})
      {
        print $fh "property $p->{name} $p->{type}\n";
        print $fh "property-hook $p->{name} $p->{hook}{function} $p->{hook}{arg_convention}\n"
          if $p->{hook};
      }

    foreach my $method (values %{$self->{class_method}})
      {
        foreach my $m (@$method)
          {
            my %m = %$m;
            print $fh join(' ', "class-method", $m{method}, "(" . join(',', @{$m{args}}) . ")", "(" . join(',', @{$m{result}}) . ")",
                           @m{qw/function has_client_arg arg_convention result_convention is_static/}) . "\n";
          }
      }

    foreach my $method (values %{$self->{instance_method}})
      {
        foreach my $m (@$method)
          {
            my %m = %$m;
            print $fh join(' ', "instance-method", $m{method}, "(" . join(',', @{$m{args}}) . ")", "(" . join(',', @{$m{result}}) . ")",
                           @m{qw/function has_client_arg arg_convention result_convention is_static/}) . "\n";
          }
      }

    return 1;
  }

sub check_valid_type
  {
    my @type = @{shift()};
    my $context = shift;

    # Every element must be a valid type
    foreach my $e (@type)
      {
        unless (exists $valid_type{$e})
          {
            die "Invalid type '$e' in $context type";
          }
      }

    # A list type must come at the end
    pop @type;
    if (grep {$_ eq '...'} @type)
      {
        die "List elements must come at the end of a type";
      }
  }

# We check for ambiguity by normalising and then comparing
sub is_ambiguous
  {
    my @a = map {$normal{$_}} @{shift()};
    my @b = map {$normal{$_}} @{shift()};

    return 0 if scalar @a != scalar @b;
    if (0 == grep {$a[$_] ne $b[$_]} 0 .. scalar @a - 1)
      {
        # There are no elements which are different. So they're ambiguous
        return 1;
      }
  }

sub add_instance_method
  {
    my $self = shift;

    my $method = shift;
    my $args = shift;
    my $result = shift;

    my $function = shift;
    my $has_client_arg = shift() ? 1 : 0;
    my $arg_convention = shift;
    my $result_convention = shift;
    my $is_static = shift() ? 1 : 0;

    check_valid_type($args, "argument");
    check_valid_type($result, "result");

    foreach (@{$self->{instance_method}{$method}})
      {
        if (is_ambiguous($_->{args}, $args))
          {
            die "Type makes method calls ambiguous, compared to $_->{function}";
          }
      }

    push @{$self->{instance_method}{$method}}, {method => $method, args => $args, result => $result,
                                                function => $function, has_client_arg => $has_client_arg,
                                                arg_convention => $arg_convention, result_convention => $result_convention,
                                                is_static => $is_static};
  }

sub add_class_method
  {
    my $self = shift;

    my $method = shift;
    my $args = shift;
    my $result = shift;

    my $function = shift;
    my $has_client_arg = shift() ? 1 : 0;
    my $arg_convention = shift;
    my $result_convention = shift;
    my $is_static = shift() ? 1 : 0;

    check_valid_type($args, "argument");
    check_valid_type($result, "result");

    foreach (@{$self->{class_method}{$method}})
      {
        if (is_ambiguous($_->{args}, $args))
          {
            die "Type makes method calls ambiguous, compared to $_->{function}";
          }
      }

    push @{$self->{class_method}{$method}}, {method => $method, args => $args, result => $result,
                                             function => $function, has_client_arg => $has_client_arg,
                                             arg_convention => $arg_convention, result_convention => $result_convention,
                                             is_static => $is_static};
  }

sub add_super
  {
    my $self = shift;
    my $super = shift;

    push @{$self->{super}}, $super;
  }

sub add_property
  {
    my $self = shift;

    my $name = shift;
    my $type = shift;

    check_valid_type([$type], "property");

    if (exists $self->{property}{$name})
      {
        die "Property $name was already declared";
      }

    $self->{property}{$name} = {name => $name, type => $type,
                               };
  }

sub add_property_hook
  {
    my $self = shift;

    my $property = shift;

    my $function = shift;
    my $arg_convention = shift;

    unless (exists $self->{property}{$property})
      {
        die "Property $property has not been declared";
      }

    if (defined $self->{property}{$property}{hook})
      {
        die "Property $property is already hooked by $self->{property}{$property}{hook}{function}";
      }

    $self->{property}{$property}{hook} = {function => $function,
                                          arg_convention => $arg_convention,
                                         };
  }

sub add_source
  {
    my $self = shift;
    my $source = shift;
    push @{$self->{source}}, $source;
  }

sub name
  {
    my $self = shift;
    return $self->{class};
  }

sub sources
  {
    my $self = shift;
    return $self->{source};
  }

sub instance_methods
  {
    my $self = shift;
    return $self->{instance_method};
  }

sub class_methods
  {
    my $self = shift;
    return $self->{class_method};
  }

sub supers
  {
    my $self = shift;
    return $self->{super};
  }

sub properties
  {
    my $self = shift;
    return $self->{property};
  }

1;
