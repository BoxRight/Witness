" Witness DSL syntax highlighting
" Language: Witness Domain Specific Language
" File: witness.vim

if exists("b:current_syntax")
  finish
endif

" Keywords
syn keyword witnessKeyword asset clause subject action object service time authority
syn keyword witnessLogical AND OR NOT IMPLIES EQUIV
syn keyword witnessType movable non_movable positive negative

" System operations
syn keyword witnessSystemOp global domain litis meet

" Join operations  
syn keyword witnessJoinOp join sell compensation consideration forbearance encumber access lien evidence argument

" Logical operations
syn keyword witnessLogicalOp oblig claim not

" Comments
syn match witnessComment "//.*$"
syn region witnessComment start="/\*" end="\*/"

" Strings
syn region witnessString start='"' end='"' skip='\\"'

" Numbers
syn match witnessNumber "\<[0-9]\+\(\.[0-9]\+\)\?\>"

" Asset definitions
syn match witnessAssetDef "asset\s\+\w\+"

" Clause definitions
syn match witnessClauseDef "clause\s\+\w\+"

" Function calls
syn match witnessFunctionCall "\w\+\s*("

" Highlighting
hi def link witnessKeyword Keyword
hi def link witnessLogical Operator
hi def link witnessType Type
hi def link witnessSystemOp Function
hi def link witnessJoinOp Function
hi def link witnessLogicalOp Function
hi def link witnessComment Comment
hi def link witnessString String
hi def link witnessNumber Number
hi def link witnessAssetDef Identifier
hi def link witnessClauseDef Identifier
hi def link witnessFunctionCall Function

let b:current_syntax = "witness" 