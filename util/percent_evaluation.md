ParamSet %-evaluation substitution macro-language
=================================================

In a nutshell
-------------

Substitution macro-language supports these kinds of variable evaluation:
* `%variable`
* `%{variable}`
* `%{variable with spaces or special chars ,!?%[}`
* `%!variable_with_only_one_leading_special_char`
* `%[scope]variable`
* `%{[scope]variable}`
* `%[scope]!variable_with_only_one_leading_special_char`

It also supports functions in the form or special variable names starting
with an equal sign and optional parameters separated by arbitrary chars the
same way sed commands do (e.g. s/foo/bar/ is the same than s,foo,bar,).
Such as `%{=date!yyyy-MM-dd}`, `%{=default:%foo:null}` or `%{=sub;foo;/o/O/g}`.

Actually, ParamSet provides such functions begining with an equal sign, and
described below, but any application (i.e. any subclass of ParamsProvider)
can implement such functions, with any prefix instead of = (or with no
prefix, or even with the same = sign although this is discouraged because it
would hide future ParamSet functions with the same name).

Variables can be the strings key-values holded by the ParamSet itself or by one
of its parents or provided through a more general ParamsProvider evaluation
context which can hold any QVariant value which will be converted to strings
when needed (and can stay typed during functions evaluation, see %=rpn below).

Detailed % syntax examples
--------------------------

*  "foo" -> "foo"
*  "%foo" -> value of param "foo" as provided by context->paramRawValue("foo")
*  "%{foo!}" -> same with param "foo!": allows special chars (excepted "}")
                special chars are any ascii char other than a-zA-Z0-9_
*  "%!foo" -> value of param "!foo": one leading special char is allowed
*  "%[bar]foo -> value of param "foo" if and only if it's in "bar" scope
*  "%{[bar]foo!}" -> same with special chars
*  "%=date" -> calling function =date: there are contextless functions (i.e.
               defined inedependently of context-provided params) and by
               convention their name always begin with =
*  "%{=date:YYYY}" -> current year in local timezone, using 4 digits
*  "%éœ§越🥨" -> value of param "éœ§越🥨": chars outside ascii are not special
*  "%{'foo}" -> "foo": a leading quote escapes param evaluation
*  "%'foo" -> "foo": remember, one leading special char is allowed
*  "%%" -> "%" : % escapes itself
*  "%{=date:%format}" -> current date using format given by "format" param
*  "%{=left:%{input}:3}" -> 3 left most utf8 characters of param "input"
*  "%{=left:abcdef:3}" -> "abc"
*  "%{=left:abcde{:3}" -> invalid: unpaired {} are not supported within {}

Scopes
------

If a scope is set then it behaves as a filter on ParamsProviders that allow it.
For instance if you have a ParamSet with scope "employee" which parent is a
ParamSet with scope "dept", and both ParamSets have a "foo" param, then if
you ask, in the context of the employee ParamSet, for `%foo` you'll get the
value for the employee whereas for `%[dept]foo` you'll get the value for the
dept.

ParamSet %-evaluation contextless functions
===========================================

The following are function that are available even whith no ParamsProvider
context, and that will prevail over any parameter set in the context (don't
call one of your params `=date` it will be hidden and replaced by current
datetime).

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
  * e %-escape value (in case it will be further %-evaluated), see %=escape
  * h html encode value, see %=htmlencode
  * u html encode will transform urls them into a links
  * n html encode will add br whenever it founds a newline

see also %=escape %' and %=htmlencode

examples:
* `%{=rawvalue!foo}` -> `%bar` if foo is `%bar`
* `%{=rawvalue!foo!e}` -> `%%bar` if foo is `%bar`
* `%{=rawvalue:h1:hun}` is equivalent to `%{=htmlencode|%{=rawvalue:h1}|un}`
* `%{'foo}` returns `foo` whereas `%{foo}` would have returned the value of foo
* `%foo` -> `%bar` if foo is `%%bar`
* `%{=escape!%{=frombase64:JWJhcg==}}` -> `%%bar` (decoded binary is `%bar`)
* `%{=escape!%foo}` -> `%%bar` if foo is `%%bar`
* `%{=rpn,foo}` -> `bar` if foo is `bar`
* `%{=rpn,'foo}` -> `foo`
* `%{=rpn,'%foo}` -> `bar` if foo is `bar` because `%foo` is evaluated as `bar`
                     by =rpn function before it's passed to MathExpr which will
                     ask for evalutation of `'bar` which won't be evaluated
                     thanks to its quote

%=ifneq
-------
`%{=ifneq!input!reference!value_if_not_equal[!value_else]}`

using %=ifneq is deprecated, it's kept for backward compatibility but one
shoud use %=switch instead.
it can be reworded as %=switch!input!reference!value_else!value_if_not_equal
or (without value_else) %=switch!input!reference!!value_if_not_equal

examples:
* `%{=ifneq:%foo:0:true:false}` -> "true" if not 0, else "false"
* `%{=switch:%foo:0:false:true}` -> same
* `%{=ifneq:%foo::notempty}` -> "notempty" if not empty, else ""
* `%{=switch:%foo:::notempty}` -> same
* `%{=ifneq:%foo::<a href="page?param=%foo">%foo</a>}` -> html link if %foo is set
* `%{=switch:%foo:::<a href="page?param=%foo">%foo</a>}` -> same

%=switch
--------
`%{=switch:input[:case1:value1[:case2:value2[...]]][:default_value]}`

test an input against different reference values and replace it according to
matching case

all parameters are evaluated, hence %foo is replaced by foo's value

if default_value is not specified, left input as is if no case matches

see also %=match, with regexps

examples:
* `%{=switch:%loglevel:E:error:W:warning:I:info:debug}`
* `%{=switch:%foo:0:false:true}` -> if 0: false else: true
* `%{=switch:%foo:0:false}` -> if 0: false else: %foo
* `%{=switch:%foo:::notempty}` -> "notempty" if not empty, else ""
* `%{=switch:%foo:::<a href="page?param=%foo">%foo</a>}` -> html link if %foo is set

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

%=left %=right
--------------
`%{=left:input:length:flags}`

`%{=right:input:length:flags}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* length is the number of character to keep from the input, if negative or
invalid, the whole input is kept
* if flags contains 'b' this is done at byte level instead of utf8 characters

examples:
* `%{=left:%foo:4}`
* `%{=left:%{=frombase64,%foo}:4:b}`
* `%{=right:%foo:4}`
* `%{=right:%{=frombase64,%foo}:4:b}`

%=mid
-----
`%{=mid:input:position[:length[:flags]]}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* position is the starting offset, 0 is first character, negatives values mean
0, values larger than the input size will produce an empty output
* length is the number of character to keep from the input, if negative or
invalid, or omitted, the whole input is kept
* if flags contains 'b' this is done at byte level instead of utf8 characters

examples:
* `%{=mid:%foo:4:5}`
* `%{=mid:%foo:4}`
* `%{=mid:%{=frombase64,%foo}:4:5:b}`

%=trim
-----
`%{=trim:input}`

removes whitespace at begining and end of input
* input is the data to transform, it is evaluated (%foo become the content of
foo param)

examples:
* `%{=trim:%foo}`
* `%{=trim:  bar}` -> "bar"

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
* `%{=htmlencode:a multiline\ntext:n}` -> `a multiline<br/>text}`
* `%{=rawvalue:h1:hun}` is equivalent to `%{=htmlencode|%{=rawvalue:h1}|un}`

%=elideright %=elideleft %=elidemiddle
--------------------------------------
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
* `%{=random}` -> any integer number (at less 32 bits, maybe more, often 64)
* `%{=random:100}` -> an integer between 0 and 99
* `%{=random:6:1}` -> an integer between 1 and 6, like a regular dice
* `%{=random:-8:-4}` -> an integer between -4 and 3

%=env
-----
`%{=env:varname1[[:varname2[:...]]:defaultvalue]}`

lookup system environment variable.

varnames, defaultValue and values of environment variables are
%-evaluated.

you must always provide a default value if there are more than 1 varname

exemples:
* `%{=env:SHELL}` -> `$SHELL` or empty if empty or not set
* `%{=env:EDITOR_FOR_%foo:vim}` -> `$EDITOR_FOR_%foo` or "vim"
* `%{=env:USERNAME:USER:}` -> `$USERNAME` or `$USER` or empty (note the trailing `:`)
* `%{=env:USERNAME:USER}` -> /!\ `$USERNAME` or "USER" (default value, not env var)
* `%{=env,EDITOR_FOR_%foo,EDITOR,vim}` -> `$EDITOR_FOR_%foo` or `$EDITOR` or "vim"

%=ext
-----
`%{=ext:namespace[[:varname2[:...]]:defaultvalue]}`

lookup external params.

namespace, varnames, defaultValue and values of external paramset are
%-evaluated.

you must always provide a default value if there are more than 1 varname

the external paramset must have been loaded before, using
registerExternalParams() function, being them loaded from csv files, external
command outputs or anything else.

loading from external command outputs is convenient to load secrets from an
external vault, using the vault command line interface, such as:
* `aws secretsmanager get-secret-value --secret-id foobar |
   jq -r '.SecretString|fromjson|. as $o|($o|keys) as $keys|$keys[]
          |[.,$o[.]]|"\""+.[0]+"\",\""+(.[1]
          |gsub("(?<sc>[,\"\\\\])";"\\"+.sc))+"\""'`

exemples:
* `%{=ext:secrets:db_password}` -> db_password value from secrets
* `%{=ext:secrets:%{=env:USER}_password:generic_password:}`
  -> $USER_password otherwise generic_password value from secrets
* `%{=ext♫secrets2♫db_password}` -> db_password value from secrets2

%=eval
------
`%{=eval!expression}`

double-evaluate expression to provide a way to force %-evaluation of a
variable (or expression) value

examples:
* `%{=eval!%foo}` -> `baz` if foo is `%bar` and bar is `baz`
* `%{=eval!%{=rawvalue:foo}}` -> very complicated equivalent of `%foo`

%=escape
--------
`%{=escape!anything}`

escape %-evaluation special characters from expression result (i.e. replace
"%" with "%%"), which is the opposite from %=eval

see also %' and %=rawvalue

examples:
* `%{=escape!%{=frombase64:JWJhcg==}}` -> `%%bar` (decoded binary is `%bar`)
* `%{=escape!%foo}` -> `%%bar` if foo is `%%bar`
* `%foo` -> `%bar` if foo is `%%bar`
* `%{'foo}` returns `foo` whereas `%{foo}` would have returned the value of foo
* `%{=rawvalue!foo}` -> `%bar` if foo is `%bar`
* `%{=rawvalue!foo!e}` -> `%%bar` if foo is `%bar`
* `%{=rpn,foo}` -> `bar` if foo is `bar`
* `%{=rpn,'foo}` -> `foo`
* `%{=rpn,'%foo}` -> `bar` if foo is `bar` because `%foo` is evaluated as `bar`
                     by =rpn function before it's passed to MathExpr which will
                     ask for evalutation of `'bar` which won't be evaluated
                     thanks to its quote

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
* flags is currently ignored

examples:
* `%{=hex:%%baz}` returns "2562617a"
* `%{=hex:%%baz: }` returns "25 62 61 7a"
* `%{=hex!%%baz!:}` returns "25:62:61:7a"
* `%{=hex:%{=fromhex!fbff61}::}` returns "fbff61"

%=fromhex
---------
`%{=fromhex!expression[!flags]}`

convert an hexadecimal representation to actual data

ignore invalid characters in input, hence tolerate separators if any

flags is currently ignored

examples:
* `%{=fromhex!2562617a!}` returns "%%baz"
* `%{=fromhex!25:62/61 7a!}` returns "%%baz"
* `%{=hex:%{=fromhex!fbff61}:}` returns "fbff61"


%=base64
--------
`%{=base64!expression[!flags]}`

base64 representation of utf-8 form of evaluated expression
* flags is a combination of letters with the following meaning:
  * u encode using base64url instead of base64 (use - and _ instead of + and /)
  * t omit trailing =

examples:
* `%{=base64:§}` returns "wqc="
* `%{=base64!%{=fromhex:fbff61}}` returns "+/9h"
* `%{=base64!%{=fromhex:fbff61}!ut}` returns "-_9h"
* `Basic %{=base64!login:password}` returns "Basic bG9naW46cGFzc3dvcmQ="

%=frombase64
------------
`%{=frombase64!expression[!flags]}`

convert a base64 representation to actual data
* flags is a combination of letters with the following meaning:
  * u decode using base64url instead of base64 (use - and _ instead of + and /)

examples:
* `%{=frombase64:wqc=}` returns "§"
* `%{=hex!%{=frombase64:+/9h}!}` returns "fbff61"
* `%{=hex!%{=frombase64:-_9h:u}!}` returns "fbff61"
* `%{=frombase64!bG9naW46cGFzc3dvcmQ=}` returns "login:password"

%=rpn
-----
`%{=rpn,term1[,term2[,...]]}`

compute a reverse polish notation mathematical expression

terms are considered as constants if they begins (and optionnaly ends) with
a simple quote and are considered variables (and will be %-evaluated) otherwise

following operators are supported with their usual (C, C++, Java, JS, bash...)
meaning:
binary operators: `+ - * / % */* .. <=> <= >= < > == != ==* !=* =~ !=~ && ^^ ||`
`?? ??* <? >? <?* >?*`
unary operators: `! !! ~ ~~ ?- !- ?* !*`
ternary operator: `?:`
please note that:
- there are no unary - and + operators
- `..` is a string concatenation operator whereas `+` is always a numerical
  operator
- `=~` is a regexp matching operator (right operand is a regexp)
- `!!` is a boolean conversion operator (`%{=rpn,1,!!}` -> true)
- `~~` is an integer conversion operator (`%{=rpn,3.14,~~}` -> 3)
- `?-` returns "false" for empty, null or invalid param and "true" otherwise
- `!-` returns the opposite
- `?*` returns "false" for null or invalid param and "true" otherwise
- `!*` returns the opposite
- `??` is a coalescence operator (`%{=rpn,',foo,??,'null,??}` -> foo value if
  not empty otherwise "null")
- `??*` is a null coalescence operator (`%{=rpn,',foo,??,'null,??*}` -> foo
  value, including empty if foo is set, event to an empty string, and otherwise
  "null"; `%{=rpn,',foo,??*}` -> always return an empty string)
- `==` and `!=\ consider non set variable or any invalid QVariant or valid
  QVariant not convertible to a number or string as if it were an empty string,
  and thus always return either true or false
- `==*` and `!=*` consider invalid QVariant or QVariant not convertible to a
  number or string as impossible to compare and return null (invalid QVariant,
  will be evaluated to an empty string if it reaches the outside of =rpn)
  whatever the value of the other operand is
- `<=> <= >= < >` will return null instead of true or false if they cannot
  their operator, that is if one is not set, invalid or impossible to convert
  to a number or a string
- `+ - *` will return null if one of their operand is not convertible to a
  number or if an integer operation overflows e.g.
  `%{=rpn,'0xffffffffffffffff','1,+}` and `%{=rpn,'1,'foo,+}` both return
  null
- `/ % */* && ^^ ||` will return null if one of their operand is not convertible
  to a number
- `*/*` is a modulo operator, like `%` but without the %-evaluation ambiguity
- `<?` and `>?` are min and max operators (`%{=rpn,'abc,'ABC,<?}` -> ABC
  and `%{=rpn,'100,~~,'20,~~,>?}` -> 100), they pretend an null, invalid or
  unconvertible operand to be an empty string
- `<?*` and `>?*` do the same but will return null as soon as one of their
  operand is null, invalid or unconvertible

some constants are also supported:
- `<null>` and `<nil>` which are synonymous and hold a null value (an invalid
  QVariant)
- `<pi>` holds Archimedes's constant

see also MathExpr which is used as %=rpn engine.
of course there are plenty of implicit type conversions, such as integer
promotions and converting non null numbers to true booleans.

examples:
* `%{=rpn,'1,'2,+}` -> 3 (addition)
* `%{=rpn,'1,'2,..}` -> "12" (concatenation)
* `%{=rpn,'1,x,+}` -> 2 if x is "1"
* `%{=rpn,'0x20,x,+}` -> 33.5 if x is "1.5"
* `%{=rpn,'2k,x,+}` -> 2001.5 if x is "1.5"
* `%{=rpn,'1,',+}` -> invalid (because 2nd operand isn't a number)
* `%{=rpn,'1,',..}` -> "1"
* `%{=rpn,'1,'true,+}` -> 2
* `%{=rpn,'1,'true,&&}` -> true (1 is casted to true by && operator)
* `%{=rpn,'1,'true,==}` -> false (1 is not a boolean)
* `%{=rpn,'42,!!,'true,==}` -> true (42 is casted to true by !! operator)
* `%{=rpn,'1,'2,==,'3,'4,?:}` -> "4"
* `%{=rpn,'aabcdaa,'a$,=~` -> true
* `%{=rpn,'aabcdaa,'c$,=~` -> false
* `%{=rpn,foo}` -> "bar" if foo is "bar"
* `%{=rpn,'foo}` -> "foo"
* `%{=rpn,'%foo}` -> "bar" if foo is "bar" because `%foo` is evaluated as `bar`
                     by =rpn function before it's passed to MathExpr which will
                     ask for evalutation of `'bar` which won't be evaluated
                     thanks to its quote
* `%{=rpn,'dt: ,=date,..}` -> "dt: " followed by current datetime
* `%{=rpn,=rpn;'42;!!,'z,..}` -> "truez" but don't do that

%'
--
`%{'anything}`

returns "anything" without evaluating it

this is usefull when an application process an input as always %-evaluated
(being in config files or elsewhere) because it make it possible to provide
a constant anyway, see %=rpn for instance

see also %=escape and %=rawvalue

examples:
* `%{'foo}` returns `foo` whereas `%{foo}` would have returned the value of foo
* `%foo` -> `%bar` if foo is `%%bar`
* `%{=escape!%{=frombase64:JWJhcg==}}` -> `%%bar` (decoded binary is `%bar`)
* `%{=escape!%foo}` -> `%%bar` if foo is `%%bar`
* `%{=rawvalue!foo}` -> `%bar` if foo is `%bar`
* `%{=rawvalue!foo!e}` -> `%%bar` if foo is `%bar`
* `%{=rpn,foo}` -> `bar` if foo is `bar`
* `%{=rpn,'foo}` -> `foo`
* `%{=rpn,'%foo}` -> `bar` if foo is `bar` because `%foo` is evaluated as `bar`
                     by =rpn function before it's passed to MathExpr which will
                     ask for evalutation of `'bar` which won't be evaluated
                     thanks to its quote