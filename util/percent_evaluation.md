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
context which can hold any TypedValue value which will be converted to strings
when needed (and can stay typed during functions evaluation, see %=rpn below).

Detailed % syntax examples
--------------------------

*  `foo` -> "foo"
*  `%foo` -> value of param "foo" as provided by context->paramRawValue("foo")
*  `%{foo!}` -> same with param "foo!": allows special chars (excepted "}")
                special chars are any ascii char other than a-zA-Z0-9_
*  `%!foo` -> value of param "!foo": one leading special char is allowed
*  `%[bar]foo` -> value of param "foo" if and only if it's in "bar" scope
*  `%{[bar]foo!}` -> same with special chars
*  `%=date` -> calling function =date: there are contextless functions (i.e.
               defined inedependently of context-provided params) and by
               convention their name always begin with =
*  `%{=date:YYYY}` -> current year in local timezone, using 4 digits
*  `%éœ§越🥨` -> value of param "éœ§越🥨": chars outside ascii are not special
*  `%%` -> "%" : % escapes itself
*  `%{=date:%format}` -> current date using format given by "format" param
*  `%{=left:%{input}:3}` -> 3 left most utf8 characters of param "input"
*  `%{=left:abcdef:3}` -> "abc"
*  `%{=left:abcde{:3}` -> invalid: unpaired {} are not supported within {}

Scope and scope filters
-----------------------

Scope and scope filters can be freely ignored, the whole percent evaluation
system works fine without any scope filter at all.

A scope name can contain any unicode character excepted comma (,), percent (%),
exclamation mark (!), colon (:), dot (.) and square brackets ([]).
Clearly curly brackets, equal or spaces are not a good idea if you want to
keep your code and logs readable, but they are supported, and so are non ascii
unicode characters (like in keys).

Filtering on scope is implementation-dependant. For instance:
* SimpleParamsProvider and RegexpParamsProvider totally ignore scopes:
  their paramRawValue() will return a value matching the key regardless the
  scope filter
* ParamsProviderMerger will select it children in their order but filtered using
  these rules:
  - an empty filter `[]` will match any child
  - a non-empty filter e.g. `[employee,boss]` won't match children with an empty
    scope or a scope not in the filter
  - a non-empty filter including an empty scope will do the same, but include
    children with an empty scope e.g. `[employee,]` won't match "boss" scope
    neither "foo" scope, but will match "" scope
  - to match only empty scopes the filter must be written `[,]` because `[]` is
    the empty filter, not a filter with the empty scope
* ParamSet will do the same than ParamsProviderMerger when climbing up it
  parents hierarchy. So `%[bar]foo` on a ParamSet with baz scope and a parent
  with bar scope will ignore foo key and let its parent provide a value.
* SharedUiItem default implementation (it can be overwritten for every type of
  item) report a scope equal to their id qualifier, so the scope can be used
  to select given types of SharedUiItem using a ParamsProviderMerger or a
  SharedUiItemList (which select its items when a scope filter is set, even
  though it has an additional selection mechanism using qualifier in the key)
* Context-less functions will totally ignore scope filters (they will match
  regardless the filter) however they propagate the filter to further evaluation
  so if `%{=default:%foo}` is evaluated in a context where the scope filter is
  `[bar]` foo value is likely to be searched only in a "bar" scoped ParamSet.
  Moreover their content is subject to regular scope filter syntax, so in
  `%{[bar]=default:%[baz]foo:%abc}` foo is filtered on baz and abc on bar,
  regardless the previous filter in the evaluation context.

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
* format defaults to pseudo-iso-8601 "yyyy-MM-dd hh:mm:ss,zzz" and uses Qt time
  and date format specifications, see:
  * https://doc.qt.io/qt/qdatetime.html#toString
  * https://doc.qt.io/qt/qdate.html#toString
  * https://doc.qt.io/qt/qtime.html#toString
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

empty coalescence function

take first non-empty expression in order: expr1 if not empty, expr2 if expr1
is empty, expr3 if neither expr1 nor expr2 are set, etc.

the function works like nvl/coalesce/ifnull functions in sql
and almost like ${variable:-value_if_not_set} in shell scripts

expr1..n are evaluated (%foo is replaced by foo's value)

see also %=utf8 and %=utf16 which does the same eval+coalesce thing but also
convert the result to a string

see also %=coalesce for null coalescence

examples:
* `%{=default!%foo!empty}` -> foo's value or "empty" instead if it's empty
* `%{=default!%foo!foo not set}`
* `%{=default:%foo:foo not set!!!}`
* `%{=default:%foo:%bar:neither foo nor bar are set!!!}`
* `%{=default!%foo!%bar}`
* `%{=default!%foo}` -> always return %foo, but won't warn if foo is not defined
* `%{=utf8:%foo}` -> "3.14" if foo holds a double == 3.14
* `%{=utf16:%foo}` -> "3.14" if foo holds a double == 3.14
* `%{=default:%foo}` -> 3.14 if foo holds a double == 3.14

%=coalesce
---------
`%{=coalesce!expr1[!expr2[...]]}`

null coalescence function

take first non-null/non-invalid/set expression in order: expr1 if not null,
expr2 if expr1 is null, expr3 if both expr1 and expr2 are null, etc.

see also %=default for empty coalescence

if unsure, use %=default instead, most of the time that's what people want

* `%{=coalesce:%foo:ø}` -> `ø` if foo is not set
* `%{=coalesce:%foo:ø}` -> "" if foo is set to ""
* `%{=coalesce:%{=rpn,<null>}:ø}` -> `ø`
* `%{=coalesce:%{=rpn,xxx,~~}:ø}` -> `ø` because xxx cannot be cast to integer
* `%{=coalesce:%foo:%bar:%baz}` -> first set variable among foo, bar, baz,
                                   or null if none is set

%=rawvalue
----------
`%{=rawvalue!variable[[!variable2[!...]]!flags]}`

return unevaluated value of a variable
* uses first non-null/non-invalid/set variable, even an empty string,
  like %=coalesce does
* flags is a combination of letters with the following meaning:
  * e %-escape value (in case it will be further %-evaluated) replace every %
      with %%
  * h html encode value, see %=htmlencode
  * u html encode will transform urls them into a links
  * n html encode will add br whenever it founds a newline

see also %=htmlencode

examples:
* `%{=rawvalue!foo}` -> `%bar` if foo is `%bar`
* `%{=rawvalue!foo!e}` -> `%%bar` if foo is `%bar`
* `%{=rawvalue:h1:hun}` is equivalent to `%{=htmlencode|%{=rawvalue:h1}|un}`
* `%foo` -> `%bar` if foo is `%%bar`
* `%{=rpn,%foo}` -> `bar` if foo is `bar`
* `%{=rpn,foo}` -> `foo`
* `%{=rpn,%%foo}` -> `%foo`
* `%{=rawvalue!notexist!baz!e}` -> `42` if baz is `42`
* `%{=rawvalue!empty!baz!e}` -> `` if empty is an empty string
* `%{=rawvalue!notexist!baz}` -> invalid whatever baz because `baz` is processed
                                 as flags

%=switch
--------
`%{=switch:input[:case1:value1[:case2:value2[...]]][:default_value]}`

test an input against different reference values and replace it according to
matching case

all parameters are evaluated, hence %foo is replaced by foo's value

if default_value is not specified, left input as is if no case matches

see also %=match, with regexps

for people knowing SQL: %=switch has exactly the same syntax than SQL decode
function (even though it's also very close to C-Java-Python-etc. switch)

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

rather use %=switch if regexps are not needed

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

support for non-standard following flags:
* ↑ convert result to upper case
* ↓ convert result to lower case

examples:
* `%{=sub!foo!/o/O}` returns "fOo"
* `%{=sub!foo!/o/O/g}` returns "fOO"
* `%{=sub;%foo;/a/b/g;/([a-z]+)[0-9]/%1%bar/g}`
* `%{=sub;2015-04-17;|.*-(?<month>[0-9]+)-.*|%month}` returns "04"
* `%{=sub!_foo_bar_!/_/-/g↑}` returns "-FOO-BAR-"

%=uppercase %=lowercase %=titlecase
-----------------------------------
`%{=uppercase:input}`

`%{=lowercase:input}`

`%{=titlecase:input}`

input is the data to transform, it is evaluated (%foo become the content of
foo param)

case folding is done at unicode level

examples:
* `%{=uppercase:fooǆ}` -> "FOOǄ"
* `%{=lowercase:Fooǆ}` -> "fooǆ"
* `%{=titlecase:fooǆ}` -> "FOOǅ"

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

%=box
-----
`{=box:input[:size[:flags[:padding[:ellipsis]]]]}`

pad and (optionnaly) elide an input text string to make it fit in a fixed size
text box

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* size is the number of characters the output must have, if negative or
  invalid, or omitted, the input is neither padded nor elided (but can still be
  trimmed)
* padding is the padding pattern, by default ' ' (one regular space character),
  it will be repeated as many times as necessary to fill in the blank,
  if empty disable padding (only enable trimming with t option and elision)
* ellipsis is the elision placeholder used when input is longer than size,
  by default, it's an empty string, but e.g. '...' (three regular ASCII dot
  characters) or '…' (unicode ellipsis character) can be convenient to make the
  ellision visible
* if flags contains 'r' pad on the right instead of left (left justify the text)
* if flags contains 'c' pad evenly on both sides (center the text) 
* if flags contains 'o' allow overflow if input is longer than size (no elision)
* if flags contains 'l' elision is done on the left instead of right
* if flags contains 'm' elision is done in the middle
* if flags contains 'b' measure sizes in bytes rather than unicode characters
* if flags contains 't' input is trimmed before padding or eliding

examples:
* `%{=box:foo:6}` -> "   foo"
* `%{=box:foo:6:r}` -> "foo   "
* `%{=box:foo:6:c}` -> " foo  "
* `%{=box:%foo:6::0}` -> "012345" if foo is "12345"
* `%{=box:%foo:8:r: }` -> "12345   " if foo is "12345"
* `%{=box:%foo:8:r:.,}` -> "12345.,." if foo is "12345"
* `%{=box:%foo:8:c: }` -> " 12345  " if foo is "12345"
* `%{=box:%foo:8::}` -> "12345" if foo is "12345" (padding disabled because of
                        empty padding parameter)
* `%{=box:  bar::t}` -> "bar" (neither padding nor elision because of invalid
                        (empty) length parameter)
* `%{=box:  bar:🥨:t}` -> "bar" (neither padding nor elision because of invalid
                          (non number) length parameter)
* `%{=box:%foo:3}` -> "123" if foo is "12345"
* `%{=box:%foo:3:l}` -> "345" if foo is "12345"
* `%{=box:%foo:3:m}` -> "145" if foo is "12345"
* `%{=box:%foo:3:m::…}` -> "1…5" if foo is "12345"
* `%{=box:%foo:4:::...}` -> "1..." if foo is "12345"
* `%{=box:%foo:4:l::...}` -> "...1" if foo is "12345"
* `%{=box:%foo:4:m::...}` -> "...1" if foo is "12345"
* `%{=box:%foo:3:::abc}` -> "abc" if foo is longer than 3
* `%{=box:%foo:3:::abcdef}` -> "abc" if foo is longer than 3

%=trim
-----
`%{=trim:input}`

removes whitespace at begining and end of input
* input is the data to transform, it is evaluated (%foo become the content of
foo param)

see also %=box which can do trimming, padding and elision at once

examples:
* `%{=trim:%foo}`
* `%{=trim:  bar}` -> "bar"
* `%{=box:  bar::t}` -> "bar"

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
`%{=elideright:input:length[:ellipsis]}`

`%{=elideleft:input:length[:ellipsis]}`

`%{=elidemiddle:input:length[:ellipsis]}`

* input is the data to transform, it is evaluated (%foo become the content of
foo param)
* length is the number of character to keep from the input, if negative or
invalid or smaller than placeholder, the whole input is kept
* ellipsis is the string replacing removed characters, by default "..." (three
  regular ASCII dot characters)

see also %=box which can do trimming, padding and elision at once

examples:
* `%{=elideright:%foo:40}`
* `%{=elideright:Hello World !:10}` -> Hello W...
* `%{=elideright:Hello World !:10:(...)}` -> Hello(...)
* `%{=elideleft:Hello World !:10}` -> ...World !
* `%{=elidemiddle:Hello World !:10}` -> Hell...d !
* `%{=box:%foo:4:::...}` -> "1..." if foo is "12345"

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
* `%{=eval:x%ooks}` -> `43` if ooks is `42` and x42 is `43`
* `%{=eval:x%ooks}` -> `43` if ooks is `baz`, baz is `42` and x42 is `43`
* `%{=eval!%{=rawvalue:foo:e}}` -> `%%bar` if foo is `%bar`
* `%{=eval!%{=rawvalue:foo}}` -> very complicated equivalent of `%foo`

%=escape
--------

%=escape seems a good idea if you don't think twice but is nonsense, it did
exist in the past and was inconsistent and removed the right ways for %-escaping
an expression are:

1) not %-evaluating it at all
2) have it already %-escaped by hand, example: `%%foo` -> `%foo`
3) `%{=rawvalue!foo!e}` -> `%%bar` if foo contains `%bar`

especially, writing this would be nonsens/inconsistent:
`%{=escape!%foo}` -> `%%bar`
because %bar would be evaluated to stay consistent with global %-evaluation
semantics, the only way to %-evaluate with a depth of only 1 is to use
%=rawvalue and its e option %-escapes the result

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

compute a reverse polish notation mathematical expression using a stack (like
HP calculators, PostScript interpreters, Forth programming language or rrdtool
formulas).
terms are %-evaluated.
see https://en.wikipedia.org/wiki/Reverse_Polish_notation

following operators are supported with their usual (C, C++, Java, JS, bash,
OCaml...) meaning:
binary operators: `+ - * / % @ @* <=> <= >= < > == != <=>* <=* >=* <* >* ==* `
`!=* =~ !=~ & ^ | && ^^ || ?? ??* <? >? <?* >?*`
unary operators: `! !! ~ ~~ ?- !- ?* !* # ##`
ternary operators: `?: ?:* :? :?*`
stack operators: `:=: <swap> <dup>`
constant operators: `<pi> <null> <nil> <nan>`
please note that:
- there are no unary - and + operators
- `@` is a concatenation operator whereas `+` is always an addition operator
  so `@` will convert numbers to strings and then concatenate them
  `@` will return null if one of its operand is null whereas `@*` will pretend
  any null (or non convertible to text) operand to be an empty string
- `=~ !=~` are regexp matching operators (top stack operand is a regexp)
- `!!` is a boolean conversion operator (`%{=rpn,1,!!}` -> true)
- `~~` is a signed 64 bits integer conversion operator (`%{=rpn,3.14,~~}` -> 3)
- `?*` not null operator: returns false if null or nan
- `!*` is null operator: returns true if null or nan
- `?-` not empty operator: returns false if empty, incl. null and nan
- `!-` is empty operator: returns true if empty, incl. null and nan
- `#` returns the size, for a string its length in characters, for a number, its
  string representation length in characters
- `##` returns the memory size, for a string its length in bytes, for a number,
  its string representation length in bytes
- `??` is an empty coalescence operator (`%{=rpn,,%foo,??,null,??}` -> foo value
  if not empty otherwise "null")
- `??*` is a null coalescence operator (`%{=rpn,<null>,%foo,??,null,??*}` -> foo
  value, including empty if foo is set, even to an empty string, and otherwise
  "null"; `%{=rpn,,%foo,??*}` -> always return an empty string)
- `== != <=> <= >= < >` consider null TypedValue or nan or TypedValue not
  convertible to a number or string as impossible to compare and return null
  whatever the value of the other operand is
- `==* !=* <=>* <=* >=* <* >*` consider non set variable or any null TypedValue
  or nan or TypedValue not convertible to a number or string as if it were an
  empty string, and thus always return either true or false
- `<=> <=>*` return a signed integer -1 0 1 or null, meaning less, equivalent,
  greater, unordered, respectively
- ?: and :? processe returns null if its test operand is null so that
  `%{=rpn,2,1,<null>,?:*}` -> null whereas ?:* and :?* process null test operand
  as if it were false so that `%{=rpn,2,1,<null>,?:}` -> 2
- :? evaluates its operands in the human natural order (test,then,else), whereas
  ?: evaluates them in the stack natural order (else,then,test) which enables
  lazy evaluation: if test is null the stack won't be evaluated below test,
  if test is false the stack won't be evaluated below then, for instance: if %x
  is true `%{=rpn,1,2,+,3,4,+,%x,?:}` will finish with a stack containing
  `1,2,+,7` and thus return 7 without evaluating the stack below it
  in the other hand `%{=rpn,%x,3,4,+,1,2,+,:?}` will finish with a stack
  containing only `7` since it must always evaluate every operand
- `+ - *` will return null if one of their operand is not convertible to a
  number or if an integer operation overflows e.g.
  `%{=rpn,0xffffffffffffffff,1,+}` and `%{=rpn,1,foo,+}` both return
  null
- `/ % && ^^ ||` will return null if one of their operands is not convertible
  to a number
- `& ^ |` will return null if one of their operands is not convertible
  to an integer
- `<?` and `>?` are min and max operators (`%{=rpn,abc,ABC,<?}` -> ABC
  and `%{=rpn,100,~~,20,~~,>?}` -> 100), and will return null as soon as one of
  their operand is null, invalid or unconvertible
- `<?*` and `>?*` do the same but will pretend a null invalid or unconvertible
  operand is an empty string
- `<null>` and `<nil>` which are synonymous and hold a null value (a null
  TypedValue)
- `<pi>` holds Archimedes' constant
- `<nan>` not a number floating point constant
- `:=:` (and its `<swap>` synonymous) swaps the two previous values in the stack
  e.g. `%{=rpn,5,4,:=:,-}` -> -1
- `<dup>` duplicates previous value e.g. `%{=rpn,4,<dup>,*}` -> 16

see also ParamsFormula class which is then engine used by %=rpn.

of course there are plenty of implicit type conversions, such as integer
promotions and converting non null numbers to true booleans.

examples:
* `%{=rpn,1,2,+}` -> 3 (addition)
* `%{=rpn,1,2,@}` -> "12" (concatenation)
* `%{=rpn,1,%x,+}` -> 2 if x is "1"
* `%{=rpn,0x20,%x,+}` -> 33.5 if x is "1.5"
* `%{=rpn,2k,%x,+}` -> 2001.5 if x is "1.5"
* `%{=rpn,1,,+}` -> null (because 2nd operand isn't a number)
* `%{=rpn,1,,@}` -> "1"
* `%{=rpn,1,true,+}` -> 2
* `%{=rpn,1,true,&&}` -> true (1 is casted to true by && operator)
* `%{=rpn,1,true,==}` -> false (1 is not a boolean)
* `%{=rpn,42,!!,true,==}` -> true (42 is casted to true by !! operator)
* `%{=rpn,2,1,==,3,4,:?}` -> "4"
* `%{=rpn,4,3,2,1,==,?:}` -> "4"
* `%{=rpn,aabcdaa,a$,=~` -> true
* `%{=rpn,aabcdaa,c$,=~` -> false
* `%{=rpn,%foo}` -> "bar" if foo is "bar"
* `%{=rpn,foo}` -> "foo"
* `%{=rpn,%%foo}` -> "%foo"
* `%{=rpn,dt: ,%=date,@}` -> "dt: " followed by current datetime
* `%{=rpn,%{=rpn;42;!!},z,@}` -> "truez"

%=utf8,=utf16
-------------
`%{=utf8:input[:input2[:...]]}`
`%{=utf16:input[:input2[:...]]}`

cast input to a string (if valid)
internaly, Utf8String and QString are used, respecitvely
if unsure, use %=utf8

examples:
* `%{=utf8:%foo}` -> "3.14" if foo holds a double == 3.14
* `%{=utf16:%foo}` -> "3.14" if foo holds a double == 3.14
* `%{=default:%foo}` -> 3.14 if foo holds a double == 3.14

%=int64,=uint64,=double,=bool
-----------------------------
`%{=int64:input[:input2[:...]]}`
`%{=uint64:input[:input2[:...]]}`
`%{=double:input[:input2[:...]]}`

cast input to a number (if possible: valid and in compatible format)
for =int64 and =uint64 in case input is a floating point value it will be
truncated toward 0
for =bool "true" "false" and numbers (anything not 0 is true) are supported

examples:
* `%{=int64:2}` -> 2
* `%{=int64:-3.14}` -> -3
* `%{=uint64:-3.14:2.71}` -> 2
* `%{=double:-3.14}` -> -3.14
* `%{=int64:blurp}` -> invalid
* `%{=int64:blurp:0}` -> 0
* `%{=int64:blurp:zero}` -> invalid
* `%{=int64:blurp:%foo:2k}` -> 2000 if foo's value cannot be converted to int

%=formatint64,=formatuint64
---------------------------
`%{=formatint64:input[:base[:padding[:default]]]}`
`%{=formatuint64:input[:base[:padding[:default]]]}`

* input: casted to a 64 bits signed (int64) or unsigned (uint64) integer
* base: default: 10
* padding: left padding pattern, e.g. "0000000000" or "        "
* default: used if input cannot be casted to an integer

examples:
* `%{=formatint64:31:16:0000}` -> `001f`
* `0x%{=formatint64:31:16}` -> `0x1f`
* `%{=formatint64:%i::%j}` -> `31` if i=`0x1f`, value of j if i=`foo`
* `%{=formatuint64:0xffffffff:16:0000000000:ø}` -> `00ffffffff`
* `%{=formatint64:2e3:16:000000:ø}` -> `002000`
* `%{=formatint64:0xffffffffffffffff:16::ø}` -> `ø`
* `%{=formatuint64:0xffffffffffffffff:16::ø}` -> `ffffffffffffffff`

%=formatdouble
--------------
`%{=formatdouble:input[:format[:precision[:default]]]}`

* input: casted to at less 64 bits floating point
* format: among "eEfFgG", default: g (choose most concise between e and f)
* precision: number of significant digits (gG formats) or after decimal point
  (eEfF formats), default: 6
* default: used if input cannot be casted to a floating point number

examples:
* `%{=formatdouble:1M:e}` -> `1.000000e+06`
* `%{=formatdouble:1::2}` -> `1.00`

%=formatboolean
--------------
`%{=formatboolean:input[:format[:default]]}`

* input: casted to boolean
* format: ignored, reserved for future usage
* default: used if input cannot be casted to a boolean

examples:
* `%{=formatboolean:1M}` -> `true`
* `%{=formatboolean:0}` -> `false`
* `%{=formatboolean:true}` -> `true`
* `%{=formatboolean:Z}` -> ``
* `%{=formatboolean:Z::false}` -> `false`

%=apply
-------
`%{=apply:variable[:param1[:param2[...]]]}`

* variable: variable name to %-evaluate with given params
* param1: value for %1 when evaluating variable
* param2: value for %2
* ...

examples:
* `%{=apply:func1:a}` -> `A` if func1 is `%{=uppercase:%1}`
* `%{=apply:func2:a:B}` -> `Ab` if func2 is `%{=uppercase:%1}%{=lowercase:%2}`
* `%{=apply:tosqlin:foo bar baz}` -> `('foo','bar','baz')`
                                     if tosqlin is `('%{=sub:%1:/ +/','/g}')`

%=color
-------
`%{=color:spec[:spec2[...]]}`

(this function is defined by libp6gui, not libp6core)
create a QColor object with given color spec, calling QColor::fromString() with
first non-empty spec

examples:
* `%{=color:red}` -> red QColor
* `%{=color:#f00}` -> red QColor
* `%{=color:%color:black}` -> red QColor if color = "red", black QColor if
                              color is empty

%=icon
------
`%{=icon!normal[!disable[!active[!selected]]]}`

(this function is defined by libp6gui, not libp6core)
create a QIcon object with given icon files paths, such QIcon object is e.g.
suitable as Qt::DecorationRole returned data from QAbstractItemModel::data()

examples:
* `%{=icon!:fas/circle-plus.svg}` -> create a QIcon object for circle-plus font
     awesome solid icon, given its embeded in a .qrc in fas/circle-plus.svg path
     (which is the case when using libsvgicons4qt's fas.qrc)
