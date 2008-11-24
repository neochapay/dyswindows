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

package YCL::Server::C;

use strict;
use 5.6.0;
use warnings;

use Carp;
use IO::File;
use YCL::YCD;

my $version = "0.1";

my %typemap = (uint32 => {decl => 'uint32_t ',
                          arg_decl => 'uint32_t',
                          re => 'uint32_t',
                          selector => '.uint32',
                         },
               ybool => {decl => 'uint32_t ',
                          arg_decl => 'uint32_t',
                          re => 'uint32_t',
                          selector => undef,
                         },
               int32 => {decl => 'int32_t ',
                         arg_decl => 'int32_t',
                         re => 'int32_t',
                         selector => '.int32',
                        },
               string => {decl => 'const char *',
                          arg_decl => 'uint32_t, const char *',
                          re => 'const\s+char\s+\*',
                          selector => undef,
                         },
               object => {decl => 'struct Object *',
                          arg_decl => 'struct Object *',
                          re => 'struct\s+Object\s+\*',
                          selector => '.obj',
                         },
               any => {decl => 'struct Value *',
                       arg_decl => 'const struct Value *',
                       re => undef,
                       selector => '',
                      },
               '...' => {decl => 'struct Tuple *',
                         arg_decl => 'const struct Tuple *',
                         re => undef,
                         selector => undef,
                        },
              );

sub scan_file
  {
    my $verbose = shift;
    my $ycd = shift;
    my $next_line = shift;

    # Now we scan the file for anything that looks like one of our
    # comments, and parse appropriately

    while (defined (my $line = $next_line->()))
      {
        if ($line =~ /^\s*\/\* METHOD\s*$/)
          {
            # Okay, next line should have the name and type
            defined ($line = $next_line->()) or next;
            unless ($line =~ /^\s*(?:\*\s+)?([A-Za-z0-9_]+)\s+::\s+\(([^\)]*)\)\s*->\s*\(([^\)]*)\)\s*$/)
              {
                die "Failed to parse method type";
                next;
              }
            my ($method, $args, $return) = ($1, $2, $3);
            my @args = split /\s*,\s*/, $args;
            my @return = split /\s*,\s*/, $return;

            # Now, we scan forward until we find the end of the comment
            while ($line = $next_line->())
              {
                last if $line =~ /\*\//;
              }
            next unless defined $line;

            # The next thing we see should be the implementation
            while ($line = $next_line->())
              {
                last if $line =~ /\S/;
              }
            next unless defined $line;

            # Storage qualifier
            my $static = $line =~ s/^\s*static\b//;

            # Then the return type
            my $type;
            if ($line =~ s/^\s*void(?:\s+|\s*$)//)
              {
                $type = 'void';
              }
            elsif ($line =~ s/^\s*struct\s+Tuple\s*\*(?:\s+|\s*$)//)
              {
                $type = 'tuple';
              }
            elsif (scalar @return == 1
                   and defined $typemap{$return[0]}{re}
                   and $line =~ s/^\s*$typemap{$return[0]}{re}(?:\s+|\s*$)//)
              {
                $type = 'direct';
              }
            else
              {
                die "Failed to parse method implementation (was expecting 'void', 'struct Tuple *', or a value type)";
              }

            # Might need to pick up another line
            while ($line !~ /\S/)
              {
                defined ($line = $next_line->()) or last;
              }
            next unless defined $line;

            # Next is the identifier
            unless ($line =~ s/\s*(\w+)\s*//)
              {
                die "Failed to parse method implementation (was expecting a function identifier)";
              }
            my $function = $1;

            # And then the argument list - this can span multiple lines
            unless ($line =~ /\(([^\)]*)(\)|$)/)
              {
                die "Failed to parse method implementation (was expecting a function argument list)";
              }

            my $arg_str = $1;

            while ($line !~ /\)/)
              {
                unless (defined ($line = $next_line->()))
                  {
                    die "Failed to parse method implementation (was expecting an close parenthesis)";
                  }
                unless ($line =~ /^\s+([^\)]*)(\)|$)/)
                  {
                    die "Failed to parse method implementation (was expecting a function argument list)";
                  }
                $arg_str .= $1;
              }

            my @function_args = split /\s*,\s*/, $arg_str;

            my $instance_method = 0;
            # It is an instance method if the first function argument is a class pointer
            if (scalar @function_args)
              {
                if ($function_args[0] =~ /^struct\s+\Q$ycd\E\s*\*\w+$/)
                  {
                    $instance_method = 1;
                    # Discard the first argument; we know everything we need to about it
                    shift @function_args;
                  }
              }

            # The next argument might be a client pointer
            my $client_arg = 0;
            if (scalar @function_args)
              {
                if ($function_args[0] =~ /^struct\s+Client\s*\*\w+$/)
                  {
                    $client_arg = 1;
                    shift @function_args;
                  }
              }

            my $argument_convention;
            # Anything left in @function_args must match the method arguments
            if (scalar @function_args == 0
                or (not $instance_method and scalar @function_args == 1 and $function_args[0] eq 'void'))
              {
                # Option 1: no function arguments
                # This is only possible if there are no method arguments
                if (scalar @args > 0)
                  {
                    die "Function/method argument types mismatch (void function, non-void method)";
                  }
                $argument_convention = 'void';
              }
            elsif (scalar @function_args == 1 and $function_args[0] =~ /^struct\s+Tuple\s*\*\w+$/)
              {
                # Diagnostic: a non-const tuple is always invalid
                die "Function has a non-const tuple argument";
              }
            elsif (scalar @function_args == 1 and $function_args[0] =~ /^const\s+struct\s+Tuple\s*\*\w+$/)
              {
                # Option 2: one tuple argument
                # This is always possible
                $argument_convention = 'tuple';
              }
            else
              {
                # Option 3: direct mapped arguments
                # This is only possible if there are method arguments
                if (scalar @args == 0)
                  {
                    die "Function/method argument types mismatch (non-void function, void method)";
                  }
                $argument_convention = 'direct';
              }

            die "Internal error: failed to determine an argument convention for '$method'" unless defined $argument_convention;

            # Likewise, for the return value
            if ($type eq 'void')
              {
                if (scalar @return > 0)
                  {
                    die "Function/method return types mismatch (void function, non-void method)";
                  }
              }

            if ($instance_method)
              {
                $ycd->add_instance_method($method, \@args, \@return,
                                          $function, $client_arg, $argument_convention, $type, $static);
              }
            else
              {
                $ycd->add_class_method($method, \@args, \@return,
                                       $function, $client_arg, $argument_convention, $type, $static);
              }
          }
        elsif ($line =~ /^\s*\/\* SUPER\s*$/)
          {
            # List of superclasses, one per line
            while ($line = $next_line->())
              {
                next if $line =~ /^\s*\*\s*$/;
                last if $line =~ /\*\//;
                unless ($line =~ /^\s*(?:\*\s+)?(\w+)\s*$/)
                  {
                    die "Failed to parse superclass declaration";
                  }
                $ycd->add_super($1);
              }
          }
        elsif ($line =~ /^\s*\/\* PROPERTY\s*$/)
          {
            # List of properties, one per line
            while ($line = $next_line->())
              {
                next if $line =~ /^\s*\*\s*$/;
                last if $line =~ /\*\//;
                unless ($line =~ /^\s*(?:\*\s+)?(\w+) :: (\w+)\s*$/)
                  {
                    die "Failed to parse property declaration";
                  }
                $ycd->add_property($1, $2);
              }
          }
        elsif ($line =~ /^\s*\/\* PROPERTY HOOK\s*$/)
          {
            # Next line should be the property name
            defined ($line = $next_line->()) or next;
            unless ($line =~ /^\s*(?:\*\s+)?(\w+)\s*$/)
              {
                die "Failed to parse property hook (was expecting a property name)";
              }
            my $property = $1;

            # Now, we scan forward until we find the end of the comment
            while ($line = $next_line->())
              {
                last if $line =~ /\*\//;
              }
            next unless defined $line;

            # The next thing we see should be the implementation
            while ($line = $next_line->())
              {
                last if $line =~ /\S/;
              }
            next unless defined $line;

            # First comes the return type
            my $type;
            $line =~ s/^\s*static\b//;
            unless ($line =~ s/^\s*void(?:\s+|\s*$)//)
              {
                die "Return type of a property hook must be void";
              }

            # Might need to pick up another line
            while ($line !~ /\S/)
              {
                defined ($line = $next_line->()) or last;
              }
            next unless defined $line;

            # Next is the identifier
            unless ($line =~ s/\s*(\w+)\s*//)
              {
                die "Failed to parse property hook implementation (was expecting a function identifier)";
              }
            my $function = $1;

            # And then the argument list
            unless ($line =~ /\(([^\)]*)\)/)
              {
                die "Failed to parse method implementation (was expecting a function argument list)";
                next;
              }
            my @function_args = split /\s*,\s*/, $1;
            my $argument_convention;

            # We'll just guess here, and emit a prototype later
            if (scalar @function_args == 0
                or (scalar @function_args == 1 and $function_args[0] eq 'void'))
              {
                $argument_convention = 'void'
              }
            elsif (scalar @function_args == 1)
              {
                $argument_convention = 'neither'
              }
            elsif (scalar @function_args == 2)
              {
                $argument_convention = 'new-only'
              }
            elsif (scalar @function_args == 3)
              {
                $argument_convention = 'both'
              }
            else
              {
                die "Failed to parse method implementation (was expecting zero to three arguments)";
              }

            $ycd->add_property_hook($property, $function, $argument_convention);
          }
        elsif ($line =~ /^\s*DEFINE_CLASS\(([^\),])\)\s*;\s*$/)
          {
            # For now, we'll reject this
            die "Found two class definitions in one file";
          }
      }
  }

sub scan
  {
    my $wanted_class = shift;
    my $verbose = shift;

    my $ycd = undef;
    my %classes;

    my $set_class = sub
      {
        my $class = shift;
        my $source = shift;

        unless ($class =~ /^\w+$/)
          {
            die "Invalid class name '$class'";
          }

        $ycd = $classes{$class} or $ycd = $classes{$class} = new YCL::YCD $class;
        $ycd->add_source($source);
      };

    foreach my $source (@_)
      {
        my $fh = new IO::File $source, "r"
          or die "Failed to open '$source': $!";

        $ycd = undef;

        # First thing we want to find is the class definition
        while (my $line = <$fh>)
          {
            chomp $line;
            if ($line =~ /^\s*DEFINE_CLASS\(([^\),]+)\)\s*;\s*$/)
              {
                if (defined $wanted_class)
                  {
                    # Not the one we want?
                    next if $1 ne $wanted_class;
                  }
                # Found it
                $set_class->($1, $source);
                last;
              }
          }

        unless (defined $ycd)
          {
            print "Nothing to do in $source. Skipping.\n" if $verbose;
            next;
          }

        my $lineno = undef;
        my $next_line = sub
          {
            my $l = <$fh>;
            if ($l)
              {
                chomp $l;
                $lineno = $fh->input_line_number;
              }
            return $l;
          };

        eval {scan_file($verbose, $ycd, $next_line);};

        if ($@)
          {
            print STDERR "$source:$lineno: $@\n";
            exit 1;
          }
      }

    return values %classes;
  }

# Brace yourself, it's about to get ugly. There's no way to mix quoted
# C and perl code that doesn't look ugly, short of using a form. And
# forms suck in perl 5. I'll rewrite it using perl 6 someday, or maybe
# using Perl6::Form in perl 5.

sub make_value
  {
    return 'list' if $_[0] eq '...';
    return lc $_[0];
  }

sub make_type
  {
    map {make_value $_} @_;
  }

#
# This is responsible for making the file ${class}.ych
# It makes the initializer available to subclasses.
# -DN
sub make_include
{
    my $ycd = shift;
    my $value = <<"END";

/*
 * Header File for $ycd
 */
#ifndef DsY_CLASS_${ycd}_YCH
#define DsY_CLASS_${ycd}_YCH

#include <Y/object/class.h>

struct ${ycd}\;

END

foreach (@{$ycd->supers})
{
 $value .= <<"END";
#include \"$_.ych\"
END
}

$value .= <<"END";
void $ycd\_init(struct $ycd *, VTable *);

#endif
END
}


sub make_header
  {
    my $class = shift;
    my $sources = shift;
    my $value = <<"END";
/****** Hey, emacs! This is a -*- C -*- file ***************************
 * Y class support code, generated by YCL::Server::C $version from:
END
    foreach my $source (@$sources)
      {
        $value .= <<"END";
 * $source
END
      }
    $value .= <<"END";
 *
 * This file contain support for the $class class.
 *
 * Since most of this file is generated from template code in yclpp,
 * it may constitute a derivative work of yclpp. Therefore:
 * Copyright (C) Andrew Suffield <asuffield\@debian.org>
 *
 * Any part of this file which is derived from yclpp may be freely
 * used, modified, and/or redistributed with or without fee. No
 * warranty is given, not even the implied warranty of merchantability
 * or fitness for a particular purpose.
 *
 * If I were not a British citizen, I would have placed these
 * components in the public domain. This license should be
 * approximately equivalent.
 *
 * Copyright for the parts which are derived from the source files
 * (named above) remains with the author of the source file. Therefore
 * this file as a whole is under the same license as the source file.
 */

#include <Y/object/class.h>
#include <Y/message/tuple.h>

#include "${class}.ych"

#define checkProperty(O, P) _Y__${class}__ ## P ## __check_property(O)
#define getProperty(O, P) _Y__${class}__ ## P ## __get_property(O)
#define safeGetProperty(O, P, U) (checkProperty(O, P) ? getProperty(O, P) : (U))
#define setProperty(O, P, V) _Y__${class}__ ## P ## __set_property(O, V)
#define CLASS_INIT ${class}_init

END
    return $value;
  }

sub make_tuple_type
  {
    my @list = make_type(@{$_[0]});
    my $count = scalar @list;
    my $value = <<"END";
      .count = $count,
      .list =
      (enum Type []){
END
        foreach my $type (@list)
          {
            $value .= <<"END";
        t_${type},
END
          }
        $value .= <<"END";
        t_undef
      }
END
    return $value;
  }

sub make_function_prototype
  {
    my $instance = shift;
    my $class = shift;
    my $function = shift;

    my $value = $function->{is_static} ? "static " : "";

    if ($function->{result_convention} eq 'void')
      {
        $value .= "void ";
      }
    elsif ($function->{result_convention} eq 'tuple')
      {
        $value .= "struct Tuple *";
      }
    elsif ($function->{result_convention} eq 'direct')
      {
        $value .= $typemap{$function->{result}[0]}{decl};
      }
    $value .= $function->{function};

    my @args;
    if ($instance)
      {
        push @args, "struct $class *";
      }

    if ($function->{has_client_arg})
      {
        push @args, "struct Client *";
      }

    if ($function->{arg_convention} eq 'tuple')
      {
        push @args, "const struct Tuple *";
      }

    if ($function->{arg_convention} eq 'direct')
      {
        push @args, map {$typemap{$_}{arg_decl}} @{$function->{args}};
      }

    push @args, 'void' unless scalar @args;

    $value .= "(" . join(', ', @args) . ")";

    $value .= ";\n";
    return $value;
  }

sub make_function_wrapper
  {
    my $instance = shift;
    my $class = shift;
    my $function = shift;

    my $value = "";
    if ($instance)
      {
        $value .= <<"END";
static instanceFunctionWrapper _Y__$function->{function}__function_wrapper;
static struct Tuple *
_Y__$function->{function}__function_wrapper(struct Object *obj, struct Client *from, const struct Tuple *args, const struct MethodType *type)
END
      }
    else
      {
        $value .= <<"END";
static classFunctionWrapper _Y__$function->{function}__function_wrapper;
static struct Tuple *
_Y__$function->{function}__function_wrapper(struct Client *from, const struct Tuple *args, const struct MethodType *type)
END
      }

    $value .= <<"END";
{
  struct Tuple *cast_args = tupleStaticCast(args, type->args);
  if (!cast_args)
    return tupleBuildError(tb_string("Type mismatch in arguments"));

END

    my @args;
    if ($instance)
      {
        push @args, "(struct $class *)obj";
      }

    if ($function->{has_client_arg})
      {
        push @args, "from";
      }

    if ($function->{arg_convention} eq 'void')
      {
      }
    elsif ($function->{arg_convention} eq 'tuple')
      {
        push @args, 'cast_args';
      }
    elsif ($function->{arg_convention} eq 'direct')
      {
        foreach my $i (0..scalar @{$function->{args}} - 1)
          {
            my $arg = $function->{args}[$i];
            if ($arg eq '...')
              {
                $value .= <<"END";
  struct Tuple tail_args = {.error = cast_args->error,
                            .count = cast_args->count - $i,
                            .list  = cast_args->list + $i};
END
                push @args, "&tail_args";
                last;
              }
            elsif ($arg eq 'string')
              {
                push @args, "cast_args->list[$i].string.len";
                push @args, "cast_args->list[$i].string.data";
              }
            else
              {
                push @args, "cast_args->list[$i]" . $typemap{$arg}{selector};
              }
          }
      }

    my $args = join(', ', @args);

    my $result_assign;
    if ($function->{result_convention} eq 'void')
      {
        $result_assign = '';
      }
    elsif ($function->{result_convention} eq 'tuple')
      {
        $result_assign = "struct Tuple *result = ";
      }
    elsif ($function->{result_convention} eq 'direct')
      {
        $result_assign = $typemap{$function->{result}[0]}{decl} . "value = ";
      }

    $value .= <<"END";
  ${result_assign}$function->{function}(${args});
END

    $value .= <<"END";
  tupleDestroy(cast_args);
END

    if ($function->{result_convention} eq 'void')
      {
        $value .= <<"END";
  return NULL;
END
      }

    if ($function->{result_convention} eq 'tuple')
      {
        # Note that we do not check the type of error tuples, because
        # they are returned out-of-band
        $value .= <<"END";

  if (result && result->error)
    return result;

  struct Tuple *cast_result = tupleStaticCast(result, type->result);
  tupleDestroy(result);

  if (cast_result)
    return cast_result;
  else
    return tupleBuildError(tb_string("Type mismatch in result"));
END
      }

    if ($function->{result_convention} eq 'direct')
      {
        my $typename = $function->{result}[0];
        $value .= <<"END";

  struct Tuple *result = tupleBuild(tb_${typename}(value));
  return result;
END
      }

    $value .= <<"END";
}

END
    return $value;
  }

sub make_method
  {
    my $instance = shift;
    my $class = shift;
    my $method = shift;
    my @functions = @{shift()};

    my $value = "";
    foreach my $function (@functions)
      {
        $value .= make_function_prototype($instance, $class, $function);
        $value .= "\n";
        $value .= make_function_wrapper($instance, $class, $function);
      }

    if ($instance)
      {
        $value .= <<"END";
static struct Tuple *
_Y__${class}__${method}__instance_wrapper(struct Object *obj, struct Client *from, const struct Tuple *args)
{
END
      }
    else
      {
        $value .= <<"END";
static struct Tuple *
_Y__${class}__${method}__class_wrapper(struct Client *from, const struct Tuple *args)
{
END
      }

    foreach my $function (@functions)
      {
        $value .= <<"END";
  struct TupleType $function->{function}_args_type =
    {
END
        $value .= make_tuple_type($function->{args});
        $value .= <<"END";
    };

  struct TupleType $function->{function}_result_type =
    {
END
        $value .= make_tuple_type($function->{result});
        $value .= <<"END";
    };

END
      }

    my $function_count = scalar @functions;

    $value .= <<"END";
  struct MethodTypes types =
    {
      .count = $function_count,
      .list =
      (struct MethodType []){
END
    foreach my $function (@functions)
      {
        $value .= <<"END";
        {
          .args = \&$function->{function}_args_type,
          .result = \&$function->{function}_result_type,
          .data = \&_Y__$function->{function}__function_wrapper
        },
END
      }
    $value .= <<"END";
        {
          .args = NULL,
          .result = NULL,
          .data = NULL
        }
      }
    };

  const struct MethodType *type = tupleMatchType(args, &types);
  if (!type)
    return tupleBuildError(tb_string("No match found for argument type"));

END
    if ($instance)
      {
        $value .= <<"END";
  instanceFunctionWrapper *wrapper = type->data;
  return (*wrapper)(obj, from, args, type);
END
      }
    else
      {
        $value .= <<"END";
  classFunctionWrapper *wrapper = type->data;
  return (*wrapper)(from, args, type);
END
      }

    $value .= <<"END";
}

END
    return $value;
  }

sub make_property_hook
  {
    my $class = shift;
    my $prop = shift;
    my $function = shift;
    my $argument_convention = shift;

    my $value = "";

    my @args;
    if ($argument_convention eq 'void')
      {
    $value .= <<"END";
static void $function(void);
END
      }
    elsif ($argument_convention eq 'neither')
      {
        push @args, "(struct $class *)obj";
    $value .= <<"END";
static void $function(struct $class *);
END
      }
    elsif ($argument_convention eq 'new-only')
      {
        push @args, "(struct $class *)obj";
        push @args, 'new';
    $value .= <<"END";
static void $function(struct $class *, const struct Value *);
END
      }
    elsif ($argument_convention eq 'both')
      {
        push @args, "(struct $class *)obj";
        push @args, 'old';
        push @args, 'new';
    $value .= <<"END";
static void $function(struct $class *, const struct Value *, const struct Value *);
END
      }

    my $args = join(', ', @args);

    $value .= <<"END";
PropertyHook _Y__${class}__${prop}__property_hook_wrapper;
void
_Y__${class}__${prop}__property_hook_wrapper(struct Object *obj, const struct Value *old, const struct Value *new)
{
  $function($args);
}

END
  }

sub make_property_wrapper
  {
    my $class = shift;
    my $prop = shift;
    my $type = shift;

    my $typename = $typemap{$type}{decl};

    my $selector;
    if ($type eq 'any')
      {
        $selector = '';
      }
    elsif ($type eq 'string')
      {
        $selector = '->string.data';
      }
    elsif ($type eq '...')
      {
        $selector = '->tuple';
      }
    else
      {
        $selector = '->' . $type;
      }

    my $lcclass = lc $class;
    my $object = "${lcclass}_to_object(obj)";

    my $value = <<"END";
static bool
_Y__${class}__${prop}__check_property(struct $class *obj)
{
  return objectGetProperty($object, "${prop}") != NULL;
}

static $typename
_Y__${class}__${prop}__get_property(struct $class *obj)
{
  return objectGetProperty($object, "${prop}")${selector};
}

static bool
_Y__${class}__${prop}__set_property(struct $class *obj, $typename value)
{
END

    if ($type eq 'string')
      {
        $value .= <<"END"
  size_t len = strlen(value);
  char str[len + 1];
  memcpy(str, value, len);

END
      }

    $value .= <<"END";
  struct Value v =
  {
    .type = t_${type},
    {
END
    if ($type eq 'string')
      {
        $value .= <<"END"
      .string = {
                  .data = str,
                  .len = len
                }
END
      }
    else
      {
    $value .= <<"END"
      .${type} = value
END
      }
    $value .= <<"END"
    }
  };
  return objectSetProperty($object, "${prop}", &v);
}

END
  }

sub output
  {
    my $class = shift;
    my $ycd = shift;
    my $output = shift;

    my $fh = new IO::File $output, "w"
      or die "Failed to open '$output' for writing: $!";

    #Make the header file 
    my $header_output = $output . 'h';
    my $header_fh = new IO::File $header_output, "w";
    print $header_fh make_include($ycd);
    close $header_fh;

    print $fh make_header($ycd, $ycd->sources);

    foreach my $method (keys %{$ycd->instance_methods})
      {
        print $fh make_method(1, $ycd, $method, $ycd->instance_methods->{$method});
      }

    foreach my $method (keys %{$ycd->class_methods})
      {
        print $fh make_method(0, $ycd, $method, $ycd->class_methods->{$method});
      }

    foreach my $prop (keys %{$ycd->properties})
      {
        print $fh make_property_wrapper($ycd, $prop, $ycd->properties->{$prop}{type});
        if ($ycd->properties->{$prop}{hook})
          {
            print $fh make_property_hook($ycd, $prop,
                                         $ycd->properties->{$prop}{hook}{function},
                                         $ycd->properties->{$prop}{hook}{arg_convention});
          }
      }

# We make the defines for the SUPER_INIT
# -DN
print $fh <<"END";
#define SUPER_INIT(O,P) \\
END
foreach (@{$ycd->supers})
{
 print $fh <<"END";
 $_\_init((struct $_ *)O,P); \\
END
}


    print $fh <<"END";

struct Class *CLASS($ycd);


static void _Y__${ycd}__class_constructor(void) __attribute__((constructor));
static void _Y__${ycd}__class_constructor(void)
{
  const char *supers[] =
    {
END
    foreach (@{$ycd->supers})
      {
        print $fh <<"END";
      "$_",
END
      }
    my $super_count = scalar @{$ycd->supers};
    print $fh <<"END";
      NULL
    };
  CLASS($ycd) = classCreate("$ycd", $super_count, supers);
END

    foreach my $method (keys %{$ycd->instance_methods})
      {
        print $fh <<"END";
  classAddInstanceMethod(CLASS($ycd), "$method", &_Y__${ycd}__${method}__instance_wrapper);
END
      }

    foreach my $method (keys %{$ycd->class_methods})
      {
        print $fh <<"END";
  classAddClassMethod(CLASS($ycd), "$method", &_Y__${ycd}__${method}__class_wrapper);
END
      }

    foreach my $prop (keys %{$ycd->properties})
      {
        my $typename = "t_" . make_value($ycd->properties->{$prop}{type});
        my $hook = $ycd->properties->{$prop}{hook} ? "&_Y__${ycd}__${prop}__property_hook_wrapper" : "NULL";

        print $fh <<"END";
  classAddProperty(CLASS($ycd), "$prop", $typename, $hook);
END
      }

    print $fh <<"END";
}
END
    close $fh;
  }
