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

package YCL::Client::Cxx;

use strict;
use 5.6.0;
use warnings;

use Carp;
use IO::File;
use YCL::YCD;

my $version = "0.1";

sub make_header
  {
    my $class = shift;
    my $sources = shift;
    my $value = <<"END";
/****** Hey, emacs! This is a -*- C++ -*- file ***************************
 * Y class support code, generated by YCL::Client::Cxx $version from:
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

#pragma implementation "${class}.yh"

#include <Y/c++/${class}.yh>

#include <Y/c++/connection.h>
#include <Y/c++/object.h>
#include <Y/c++/widget.h>
#include <Y/c++/value.h>

#include <string>
#include <utility>

END
    return $value;
  }

my %typemap = (uint32 => 'uint32_t',
               int32 => 'int32_t',
               ybool => 'ybool',
               string => 'std::string',
               object => 'Y::Object'
              );

my %type_decl = (uint32 => 'uint32_t ',
                 int32 => 'int32_t ',
                 ybool => 'ybool',
                 string => 'const std::string& ',
                 object => 'Y::Object& ',
                 '...' => 'const Y::Message::Members& '
                );

sub make_args_decl
  {
    my $args = shift;
    my $i = 1;
    my @args = map {$type_decl{$_} . "arg" . $i++} @$args;
    push @args, 'void' unless scalar @args;
    return join(', ', @args);
  }



sub make_result_decl
  {
    my $result = shift;
    if (scalar @$result == 0)
      {
        return ('void', undef);
      }
    elsif (scalar @$result == 1)
      {
        my $type = $typemap{$result->[0]};
        return ("Y::Value<$type> *", $type);
      }
    elsif (scalar @$result == 2)
      {
        my $type = "std::pair<$typemap{$result->[0]}, $typemap{$result->[1]}>";
        return ("Y::Value<$type > *", $type);
      }
    else
      {
        return ('Y::Reply *', undef);
      }
  }


# Creates the getters and setters
# takes input:
# class, propertyname, propertytype
sub make_getter_setter
  {
    my $class = shift;
    my $prop = shift;
    my $proptype = shift;
    
    if ($proptype eq "ybool")
      {
        $proptype = "bool";
      }
    
    my $propname = ucfirst $prop;
    
    my $value = "\n";

    $value .= <<"END";
/*
 * Getter for property $prop
 * Return type $proptype
 */
$proptype  
Y::ServerObject::${class}::get$propname (void)
{
END
  if ( $proptype eq 'bool' )
    {
      $value .= <<"END";
  Y::Value<ybool> *temp = $prop.get();
  ybool val = temp->value();
  if (val)
    return true;
  else
    return false; 
END
    }
  else
    {
    $value .= <<"END";
  Y::Value<$proptype> *temp = $prop.get(); 
  return temp->value();
END
    }
    $value .= <<"END";
}

/*
 * Setter for property $prop
 * Property type $proptype
 */
void  
Y::ServerObject::${class}::set$propname ($proptype p)
{
END
  if ( $proptype eq 'bool' )
   {
     $value .= <<"END";
  if (p)
    $prop.set(1);
  else
    $prop.set(0);
END
   }
  else
   {
     $value .= <<"END";
  $prop.set(p); 
END
   }
  $value .= <<"END";
}
END
    
    return $value;
  }

#
# This makes a method which returns an Object *
# right now object returns are treated as a special case,
# I suspect there will be other special cases when we get into more widget code.
#
sub make_objreturn_method
  {
    my $class = shift;
    my $method = shift;
    my $args = shift;
    my $args_decl = make_args_decl($args);
    
    my $value = "";
    
    $value .=<<"END";
/*
 * This method was autogenerated.
 * Please remember that methods which return Objects assume the
 * client end (thats you) created the requested object in the first place.
 *
 * What I mean is..
 * If you are looking at this now because you just wrote a fancy server widget
 * which internally creates an object on the server and you want the client to 
 * be able to get at it, so you wrote a nifty "getObject" method, it ain't gonna 
 * work--maybe it will someday in the future, but I doubt it..
 * -DN
 */
     
Y::Object *
Y::ServerObject::${class}::${method} ($args_decl)
{
  Y::Message::Members v;
  v.push_back("$method");
END

    foreach my $i (1..scalar @$args)
      {
        if ($args->[$i-1] eq '...')
          {
            $value .= <<"END";
  v.insert(v.end(), arg$i.begin(), arg$i.end());
END
          }
        else
          {
            $value .= <<"END";
  v.push_back(arg$i);
END
	  }
      }
      
      $value .= <<"END";
  Reply *r = invokeMethod (v, true);
  uint32_t temp = r->id();
  return y->findObject(temp);
}
END
  }#end make_objreturn_method

sub make_method
  {
    my $class = shift;
    my $method = shift;
    my $args = shift;
    my $result = shift;
    
    #check if this should be an object method
    if (scalar @$result == 1 && $result->[0] eq 'object')
      {
        return make_objreturn_method($class, $method, $args);
      }
    
    my $args_decl = make_args_decl($args);
    my ($ret_decl, $ret_type) = make_result_decl($result);

    my $value = "";

    $value .= <<"END";

$ret_decl
Y::ServerObject::${class}::${method} ($args_decl)
{
  Y::Message::Members v;
  v.push_back("$method");
END

    foreach my $i (1..scalar @$args)
      {
        if ($args->[$i-1] eq '...')
          {
            $value .= <<"END";
  v.insert(v.end(), arg$i.begin(), arg$i.end());
END
          }
        else
          {
            $value .= <<"END";
  v.push_back(arg$i);
END
}
      }

    if (scalar @$result > 2)
      {
    $value .= <<"END";
  Reply *r = invokeMethod (v, true);
  return r;
END
      }
    elsif (scalar @$result > 0)
      {
        die "Internal error: no return type" unless defined $ret_type;
    $value .= <<"END";
  Reply *r = invokeMethod (v, true);
  return new Y::Value<$ret_type >(r);
END
      }
    else
      {
    $value .= <<"END";
  invokeMethod (v, false);
END
      }

    $value .= <<"END";
}
END

    return $value;
  }

sub output
  {
    my $class = shift;
    my $ycd = shift;
    my $output = shift;

    my $fh = new IO::File $output, "w"
      or die "Failed to open '$output' for writing: $!";

    print $fh make_header($ycd, $ycd->sources);

    # Super classes
    my @parents;
    my @parents_protected;
    foreach my $parent ( @{$ycd->supers} )
    {   
      push @parents, ("Y::" . $parent . "(y, \"" . $ycd . "\")"); 
      push @parents_protected, ("Y::" . $parent . "(y, className)");
    }
    
    my @init_list;
    foreach my $prop (keys %{$ycd->properties})
      {
        push @init_list, "$prop(this, \"$prop\")";
      }

    my $init_list = join(",\n   ", @parents, @init_list);
    my $init_list_protected = join(",\n   ", @parents_protected, @init_list);
    
    print $fh <<"END";

Y::ServerObject::${ycd}::${ycd} (Y::Connection *y)
 : $init_list
{
}

Y::ServerObject::${ycd}::${ycd} (Y::Connection *y, std::string className)
 : $init_list_protected
{
}

Y::ServerObject::${ycd}::~${ycd} ()
{
}

bool
Y::ServerObject::${ycd}::onEvent (const std::string &name, const Y::Message::Members& params)
{
END

    print $fh <<"END";
  return false;
}
END

    foreach my $method (keys %{$ycd->instance_methods})
      {
        foreach my $m (@{$ycd->instance_methods->{$method}})
          {
            print $fh make_method($ycd, $method, $m->{args}, $m->{result});
          }
      }

    # Generates the getters and setters
    foreach my $prop (keys %{$ycd->properties})
      {
        my $type = $typemap{$ycd->properties->{$prop}{type}};
        print $fh make_getter_setter ($ycd, $prop, $type);
      }
    
    close $fh;
  }