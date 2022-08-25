ParamSet %-evaluation substitution macro-language
=================================================

Substitution macro-language supports these kinds of variable evaluation:
* `%variable`
* `%{variable}`
* `%{variable with spaces or special chars ,!?%}`
* `%!variable_with_only_one_leading_special_char`

It also supports functions in the form or special variable names starting
with an equal sign and optional parameters separated by arbitrary chars the
same way sed commands do (e.g. s/foo/bar/ is the same than s,foo,bar,).
Such as `%{=date!yyyy-MM-dd}`, `%{=default:%foo:null}` or `%{=sub;foo;/o/O/g}`.

Actually, ParamSet provides such functions begining with an equal sign, and
described below, but any application (i.e. any subclass of ParamsProvider)
can implement such functions, with any prefix instead of = (or with no
prefix, or even with the same = sign although this is discouraged because it
would hide future ParamSet functions with the same name).

ParamSet %-evaluation functions
===============================

%=date
------
`%{=date!format!relativedatetime!timezone}`

formats a timestamp or a relative time reference
* format defaults to pseudo-iso-8601 "yyyy-MM-dd hh:mm:ss,zzz"
* relativedatetime defaults to current date time
* timezone defaults to local time, if specified it must follow IANA's timezone
format, see http://www.iana.org/time-zones

see https://gitlab.com/g76r/libp6core/-/blob/master/util/relativedatetime.h for
an extended documentation of relative time reference supported syntax

examples:
* `%=date`
* `%{=date!yyyy-MM-dd}`
* `%{=date,yyyy-MM-dd}`
* `%{=date!!-2days}`
* `%{=date!!!UTC}`
* `%{=date,,,UTC}`
* `%{=date!hh:mm:ss,zzz!01-01T20:02-2w+1d!GMT}`
* `%{=date!iso}` short for `%{=date!yyyy-MM-ddThh:mm:ss,zzz}`
* `%{=date!ms1970}` milliseconds since 1970-01-01 00:00:00,000
* `%{=date!s1970}` seconds since 1970

%=coarsetimeinterval
--------------------
`%{=coarsetimeinterval!seconds}`

formats a time interval as a coarse human readable expression

examples:
* `%{=coarsetimeinterval:1.250}` -> "1.250 seconds"
* `%{=coarsetimeinterval:125.35}` -> "2 minutes 5 seconds"
* `%{=coarsetimeinterval:86402.21}` -> "1 days 0 hours"

%=default
---------
`%{=default!expr1[!expr2[...]]}`

take first non-empty expression in order: expr1 if not empty, expr2 if expr1
is empty, expr3 if neither expr1 nor expr2 are set, etc.

the function works like nvl/coalesce/ifnull functions in sql
and almost like ${variable:-value_if_not_set} in shell scripts

expr1..n are evaluated (%foo is replaced by foo's value)

examples:
* `%{=default!%foo!null}` -> foo's value or "null" instead if it's empty
* `%{=default!%foo!foo not set}`
* `%{=default:%foo:foo not set!!!}`
* `%{=default:%foo:%bar:neither foo nor bar are set!!!}`
* `%{=default!%foo!%bar}`
* `%{=default!%foo}` -> always return %foo, but won't warn if foo is not defined

%=rawvalue
----------
`%{=rawvalue!variable[!flags]}`

return unevaluated value of a variable
* flags is a combination of letters with the following meaning:
  * e %-escape value (in case it will be further %-evaluated)
  * h html encode value
  * u html encode will transform urls them into a links
  * n html encode will add br whenever it founds a newline

examples:
* `%{=rawvalue!foo}`
* `%{=rawvalue!foo!hun}`
* `%{=rawvalue!foo!e}` is an equivalent to %{=escape!%foo}


%=ifneq
-------
`%{=ifneq!input!reference!value_if_not_equal[!value_else]}`

test inequality of an input and replace it with another
depending on the result of the test

all parameters are evaluated, hence %foo is replaced by foo's value

examples:
* `%{=ifneq:%foo:0:true:false}` -> "true" if not null, else "false"
* `%{=ifneq:%foo::notempty}` -> "notempty" if not empty, else ""
* `%{=ifneq:%foo::<a href="page?param=%foo">%foo</a>}` -> html link if %foo is set

%=switch
--------
`%{=switch:input[:case1:value1[:case2:value2[...]]][:default_value]}`

test an input against different reference values and replace it according to
matching case

all parameters are evaluated, hence %foo is replaced by foo's value

if default_value is not specified, left input as is if no case matches

see also %=match, with regexps

%=switch can be used as would be an %=ifeq function since those two lines
are strictly equivalent:
* `%{=switch:value:case1:value1[:default_value]}`
* `%{=switch:value:reference:value_if_equal[:value_if_not_equal]}`

examples:
* `%{=switch:%loglevel:E:error:W:warning:I:info:debug}`
* `%{=switch:%foo:0:false:true}` -> if 0: false else: true
* `%{=switch:%foo:0:false}` -> if 0: false else: %foo
* `%{=switch:%foo}` -> always return %foo, but won't warn if foo is not defined

%=match
-------
`%{=match:input[:regexp1:value1[:regexp2:value2[...]]][:default_value]}`

test an input against different reference regexps and replace it according to
matching case

all parameters are evaluated, hence %foo is replaced by foo's value

if default_value is not specified, leave input as is if no case matches

see also %=switch if regexps are not needed

examples:
* `%{=match:%foo:^a:false:true}` -> if starts with 'a': `false` else: `true`
* `%{=match:%foo:[0-9]+:true}` -> if only digits: `true` else: `%foo`
* `%{=match:%foo}` -> always return %foo, but won't warn if foo is not defined

%=sub
-----
`%{=sub!input!s-expression!...}`

input is the data to transform, it is evaluated (%foo become the content of
foo param)

s-expression is a substitution expression like those taken by sed's s
command, e.g. /foo/bar/gi or ,.*,,

replacement is evaluated and both regular params substitution and
regexp substitution are available, e.g. %1 will be replaced by first
capture group and %name will be replaced by named capture group if
availlable or param

regular expressions are those supported by Qt's QRegularExpression, which
are PCRE-powered, i.e. almost identical to Perl's regexps

examples:
* `%{=sub!foo!/o/O}` returns "fOo"
* `%{=sub!foo!/o/O/g}` returns "fOO"
* `%{=sub;%foo;/a/b/g;/([a-z]+)[0-9]/%1%bar/g}`
* `%{=sub;2015-04-17;|.*-(?<month>[0-9]+)-.*|%month}` returns "04"

%=left
------
`%{=left:input:length}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* length is the number of character to keep from the input, if negative or
invalid, the whole input is kept

examples:
* `%{=left:%foo:4}`

%=right
-------
`%{=right:input:length}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* length is the number of character to keep from the input, if negative or
invalid, the whole input is kept

examples:
* `%{=right:%foo:4}`

%=mid
-----
`%{=mid:input:position[:length]}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* position is the starting offset, 0 is first character, negatives values mean
0, values larger than the input size will produce an empty output
* length is the number of character to keep from the input, if negative or
invalid, or omitted, the whole input is kept

examples:
* `%{=mid:%foo:4:5}`
* `%{=mid:%foo:4}`

%=htmlencode
------------
`%{=htmlencode:input[:flags]}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param) and cannot contain the separator character (e.g. :)
* flags can contain following characters:
  * u to surround url with links markups
  * n to add br markup whenever a newline is encountered

examples:
* `%{=htmlencode:1 < 2}` -> `1 &lt; 2`
* `%{=htmlencode,http://wwww.google.com/,u}` -> `<a href="http://wwww.google.com/">http://wwww.google.com/</a>`
* `%{=htmlencode http://wwww.google.com/ u}` -> same
* `%{=htmlencode|http://wwww.google.com/}` -> `http://wwww.google.com/`

%=elidexxx
----------
`%{=elideright:input:length[:placeholder]}`

`%{=elideleft:input:length[:placeholder]}`

`%{=elidemiddle:input:length[:placeholder]}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* length is the number of character to keep from the input, if negative or
invalid or smaller than placeholder, the whole input is kept
* placeholder is the string replacing removed characters, by default "..."

examples:
* `%{=elideright:%foo:40}`
* `%{=elideright:Hello World !:10}` -> Hello W...
* `%{=elideright:Hello World !:10:(...)}` -> Hello(...)
* `%{=elideleft:Hello World !:10}` -> ...World !
* `%{=elidemiddle:Hello World !:10}` -> Hell...d !

%=random
--------
`%{=random[:modulo[:shift]]`

produce a pseudo-random integer number between shift (default: 0) and
modulo-1+shift.

negative modulos are silently converted to their absolute values.

examples:
* `%{=random}` -> any integer number (at less 32 bits, maybe more)
* `%{=random:100}` -> an integer between 0 and 99
* `%{=random:6:1}` -> an integer between 1 and 6, like a regular dice
* `%{=random:-8:-4}` -> an integer between -4 and 3

%=env
-----
`%{=env:varname1[[:varname2[:...]]:defaultvalue]}`

lookup system environment variable.

varnames and defaultValue are evaluated.

values of env vars themselves are not evaluated (USER=%foo will remain %foo),
but you can still use %=eval if needed (see examples below).

you must provide a default value if there are more than 1 varname

exemples:
* `%{=env:SHELL}` -> `$SHELL` or empty if empty or not set
* `%{=env:EDITOR_FOR_%foo:vim}` -> `$EDITOR_FOR_%foo` or "vim"
* `%{=env:USERNAME:USER:}` -> `$USERNAME` or `$USER` or empty (note the trailing `:`)
* `%{=env:USERNAME:USER}` -> /!\ `$USERNAME` or "USER" (default value, not env var)
* `%{=env,EDITOR_FOR_%foo,EDITOR,vim}` -> `$EDITOR_FOR_%foo` or `$EDITOR` or "vim"
* `%{=eval!%{=env:FOO}}` -> %-evaluated $FOO value

%=eval
------
`%{=eval!expression}`

double-evaluate expression to provide a way to force %-evaluation of a
variable (or expression) value

examples:
* `%{=eval!%foo}` -> `baz` if foo is `%bar` and bar is `baz`
* `%{=eval!%{=env:FOO}}` -> %-evaluated value of $FOO environment variable
* `%{=eval!%{=rawvalue:foo}}` -> very complicated equivalent of `%foo`

%=escape
--------
`%{=escape!expression}`

escape %-evaluation special characters from expression result (i.e. replace
"%" with "%%"), which is the opposite from %=eval

examples:
* `%{=escape!%foo}` -> `%%bar` if foo is `%bar`
* `{=rawvalue!foo!e}` -> equivalent to `%{=escape!%foo}`
* `%{=escape!%foo-%baz}` -> `%%bar-42` if foo is `%bar` and baz is `42`
* `%{=eval:%{=escape!%foo}}` -> very complicated equivalent of `%{=rawvalue:foo}`

%=sha1
------
`%{=sha1!expression}`

compute sha1 of evaluated expression

examples:
* `%{=sha1:%%baz}` -> "3d8555b0a81f8344fd128060117b985ce9de6bd5"

%=sha256
--------
`%{=sha256!expression}`

compute sha256 of evaluated expression

examples:
* `%{=sha256:%%baz}` -> "48b56c9eb1d1d80188aeda808c72a047cd15803c57117bec272c75145f84f525"


%=md5
-----
`%{=md5!expression}`

compute md5 of evaluated expression

examples:
* `%{=md5:%%baz}` -> "96ab86a37cef7e27d8d45af9c29dc974"

%=hex
-----
`%{=hex!expression[!separator[!flags]]}`

hexadecimal representation of utf-8 form of evaluated expression
* separator optionnal one-latin1-character separator between bytes
* flags is a combination of letters with the following meaning:
  * b process expression value as binary, not utf-8

examples:
* `%{=hex:%%baz}` returns "2562617a"
* `%{=hex:%%baz: }` returns "25 62 61 7a"
* `%{=hex!%%baz!:}` returns "25:62:61:7a"
* `%{=hex:%{=fromhex!fbff61!b}::b}` returns "fbff61"
* `%{=hex:%{=fromhex!fbff61}::b}` returns "3f3f61" (\x3f is '?' placeholder)

%=fromhex
---------
`%{=fromhex!expression[!flags]}`

convert an hexadecimal representation to actual data

ignore invalid characters in input, hence tolerate separators if any

flags is a combination of letters with the following meaning:
* b produces binary result, not utf-8

examples:
* `%{=fromhex!2562617a!}` returns "%%baz"
* `%{=fromhex!25:62/61 7a!}` returns "%%baz"
* `%{=hex:%{=fromhex!fbff61!b}::b}` returns "fbff61"
* `%{=hex:%{=fromhex!fbff61}::b}` returns "3f3f61" (\x3f is '?' placeholder)


%=base64
--------
`%{=base64!expression[!flags]}`

base64 representation of utf-8 form of evaluated expression
* flags is a combination of letters with the following meaning:
  * b process expression value as binary, not utf-8
  * u encode using base64url instead of base64 (use - and _ instead of + and /)
  * t omit trailing =

examples:
* `%{=base64:§}` returns "wqc="
* `%{=base64!%{=fromhex:fbff61:b}!b}` returns "+/9h"
* `%{=base64!%{=fromhex:fbff61:b}!utb}` returns "-_9h"
* `Basic %{=base64!login:password}` returns "Basic QmFzaWMgbG9naW46cGFzc3dvcmQ="

%=frombase64
------------
`%{=frombase64!expression[!flags]}`

convert a base64 representation to actual data
* flags is a combination of letters with the following meaning:
  * b produces binary result, not utf-8
  * u decode using base64url instead of base64 (use - and _ instead of + and /)

examples:
* `%{=frombase64:wqc=}` returns "§"
* `%{=hex!%{=frombase64:+/9h:b}!!b}` returns "fbff61"
* `%{=hex!%{=frombase64:-_9h:ub}!!b}` returns "fbff61"
* `%{=frombase64!QmFzaWMgbG9naW46cGFzc3dvcmQ=}` returns "login:password"

%=rpn
-----
`%{=rpn,term1[,term2[,...]]}`

compute a reverse polish notation mathematical expression
terms are considered as constants if the begins (and optionnaly ends) with
a simple quote and are considered variables (and will be %-evaluated) otherwise

following operators are supported with there usual (C, C++, Java...) priorities
binary operators: `+ - * / % */* .. <=> <= >= < > == != ~= && ^^ ||`
unary operators: `! !! ~`
ternary operator: `?:`
please note that:
- there are no unary - and + operators
- .. is a string concatenation operator whereas + is always a numerical operator
- ~= is a regexp matching operator (right operand is a regexp)
- !! is a shortcut for boolean conversion (%{=rpn,1,!!} -> true)

see also MathExpr which operates with QVariant args and not only strings and is
used as %=rpn engine. of course there are plenty of implicit type conversions,
such as integer promotions and converting non null numbers to true booleans.

examples:
* `%{=rpn,'1','2',+}` returns "3"
* `%{=rpn,'1,'2,..}` returns "12"
* `%{=rpn,'1,x,+}` returns "2" if x is "1"
* `%{=rpn,'0x20,x,+}` returns "33.5" if x is "1.5"
* `%{=rpn,'1,',+}` returns "" (because second operand is not a number)
* `%{=rpn,'1,'true,+}` returns "2"
* `%{=rpn,'1,'true,&&}` returns "true"
* `%{=rpn,'1,'true,==}` returns "false"
* `%{=rpn,'42,!!,'true,==}` returns "true"
* `%{=rpn,'1,'2,==,'3,'4,?:}` returns "4"
* `%{=rpn,'aabcdaa,'a$,~=` returns "true"
* `%{=rpn,'aabcdaa,'c$,~=` returns "false"
