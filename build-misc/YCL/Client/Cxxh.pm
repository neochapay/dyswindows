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

package YCL::Client::Cxxh;

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

#pragma interface "${class}.yh"

#include <Y/c++/connection.h>
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
                 string => 'const std::string &',
                 object => 'Y::Object& ',
                 '...' => 'const Y::Message::Members&'
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
        # Deal with object special case
        if ($result->[0] eq 'object') 
          {
            return ("Y::Object *");
          }
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

sub output
  {
    my $class = shift;
    my $ycd = shift;
    my $output = shift;

    my $fh = new IO::File $output, "w"
      or die "Failed to open '$output' for writing: $!";

    print $fh make_header($ycd, $ycd->sources);
    # need to add the headers for the super classes
    foreach my $super_headers (@{$ycd->supers})
    {
      my $hdr = lc $super_headers;
      print $fh <<"END";
#include <Y/c++/objects/${hdr}.h>
END
    }

# Super classes..
    my $parent = join (",\n\t\tY::", @{$ycd->supers});
    $parent = join ("::", 'Y', $parent);
    print $fh <<"END";
    
    
namespace Y
{
  namespace ServerObject
  {
    class $ycd : public $parent
    {
    public:
      $ycd (Y::Connection *y);
      virtual ~$ycd ();
      
     /*
      * Autogenerated Getters and Setters
      */
END

     foreach my $prop (keys %{$ycd->properties})
      {
        my $type = $typemap{$ycd->properties->{$prop}{type}};
        my $propname = ucfirst $prop;
        if ($type eq 'ybool')
         {
           $type = 'bool';
         }
        print $fh <<"END";
      $type get$propname (void);
      void set$propname ($type);
END
      }
    print $fh <<"END";
    
     /*
      * Instance Methods
      */
END
    foreach my $method (keys %{$ycd->instance_methods})
      {
        foreach my $m (@{$ycd->instance_methods->{$method}})
          {
            my $args = make_args_decl($m->{args});
            my ($ret, undef) = make_result_decl($m->{result});
        print $fh <<"END";
      $ret $method ($args);
END
          }
      }

    print $fh <<"END";

    protected:
      $ycd (Y::Connection *y, std::string className);
      virtual bool onEvent (const std::string &, const Y::Message::Members&);

    private:
END
     foreach my $prop (keys %{$ycd->properties})
      {
        my $type = $ycd->properties->{$prop}{type};
        print $fh <<"END";
      Object::Property<$typemap{$type}> $prop;
END
      }
    print $fh <<"END";
    };
  }
}
END

    close $fh;
  }
