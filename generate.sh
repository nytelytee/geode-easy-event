#!/bin/sh

VAR="EASY_EVENT_PRETTIFY_MACROS_IS_THIS_NAME_LONG_ENOUGH_TO_NOT_ACCIDENTALLY_BE_DEFINED_BY_SOMEONE_ELSE_YET"

INDENT="      "

sed -n '/\/\*\*\*\* START MACRO SEQUENCE \*\*\*\*\//q;p' ./easy-event.hpp

sed -n '/\/\*\*\*\* START MACRO SEQUENCE \*\*\*\*\//,/\/\*\*\*\* END MACRO SEQUENCE \*\*\*\*\//{
/\/\*\*\*\* START MACRO SEQUENCE \*\*\*\*\//d
/\/\*\*\*\* END MACRO SEQUENCE \*\*\*\*\//d
p
}' ./easy-event.hpp |
	# pipe it to clang to expand the macros
	clang -E -dI -C -P -CC -D ${VAR}= - |
	# replace EE_NL with a newline
	sed -e "s/EE_NL/\n/g" |
	# perform text replacements to make this look more readable
	sed \
		-e "s/ typename Function/typename Function/" \
		-e "s/ typename Class/typename Class/" \
		-e "s/result , typename/result, typename/" \
		-e "s/result , Function/result, Function/" \
		-e "s/WithID< Function>/WithID<Function>/" \
		-e "s/WithIDOn< Class>/WithIDOn<Class>/" \
		-e "s/WithID<result , Function>/WithID<result, Function>/" \
		-e "s/WithIDOn<result , Class>/WithIDOn<result, Class>/" \
		-e "s/( Function/(Function/" \
		-e "s/( Class/(Class/" \
		-e "s/(const char\* id , Function/(const char* id, Function/" \
		-e "s/(const char\* id , Class/(const char* id, Class/" \
	|
	# luckily, everything has the same indentation level, so we can just replace all the
	# spaces from clang's output with whatever the indent is, without any special logic
	sed -e "s/^ */$INDENT/" |
	# remove trailing spaces
	sed -e "s/[ 	]*$//"

sed -n '/\/\*\*\*\* END MACRO SEQUENCE \*\*\*\*\//,${/\/\*\*\*\* END MACRO SEQUENCE \*\*\*\*\//!p}' ./easy-event.hpp
