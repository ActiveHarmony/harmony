grammar PeriCSL;

/*
	Author: Ananta Tiwari
	University of Maryland at College Park
	E-mail: tiwari@cs.umd.edu
	
	This is an ANTLR grammar file for PERI Constraint Specification Language (PERI-CSL).
	
	This is a prototype version - or more appropriately, my first take on using ANTLR. So - an
	ANTLR expert will probably find many ways to make this look and work better. Any suggestions
	are welcome.

*/

options {
	k=5;
	output=AST;
    	ASTLabelType=CommonTree;
    	backtrack=true;
    	memoize=true;
}

tokens {
	SPACE; SPACE_BODY; SPACE_NAME; CONSTANTS; ASSIGN; CONST_ASSIGN; CODE_REGION; REGION_SET; REGION_SET_NAME;
	SLIST; PARAMETERS; PARAMETER_DECL; TYPE; RANGE; PRANGE; INTARR; INTRANGE; FLOATRANGE; INTPRANGE; FLOATPRANGE; 
	FLOATARR; MIXEDARR; BOOLARR; STRARR; DEFAULT; REGION_SPEC; CONSTRAINT; CONSTRAINT_NAME; 
	CONSTRAINT_PARAMS; EXPR; NEGATE; REGION_REF; PARAM_REF; ARR_ENUM_REF; SPECIFICATION; GROUPED_SPEC; CONJ;
	TBOOL; ARR; FLATARR; ARROFARR;
}

@header {
package org.umd.periCSL;

import java.util.HashMap;
import java.util.Vector;
import java.util.HashSet;
import java.util.Iterator;
}

@lexer::header {
package org.umd.periCSL;
}

@members {
/*
	ERROR REPORTING CODE
*/
/*
To get clean error messages, override the following three methods.
*/
public String getErrorMessage(RecognitionException e,
                              String[] tokenNames)
{
    List stack = getRuleInvocationStack(e, this.getClass().getName());
    String msg = null;
    
    if ( e instanceof NoViableAltException ) 
    {
       NoViableAltException nvae = (NoViableAltException)e;
       msg = " no viable alt; token="+e.token+
          " (decision="+nvae.decisionNumber+
          " state "+nvae.stateNumber+")"+
          " decision=<<"+nvae.grammarDecisionDescription+">>";
    }
    else 
    {
       msg = super.getErrorMessage(e, tokenNames);
    }
    return stack+" "+msg;
}

public String getTokenErrorDisplay(Token t) 
{
    return t.toString();
}

public void displayRecognitionError(String[] tokenNames,
                                        RecognitionException e) 
{
        String hdr = getErrorHeader(e);
        String msg = getErrorMessage(e, tokenNames);
        System.err.println("Error Location in the input file:: " + hdr);
        System.err.println(msg);
        System.exit(1);
}

/*
	ERROR REPORTING CODE DONE
*/

/*
	TEMPS
*/
ArrayList<String> code_regions=new ArrayList<String>();
HashMap<String, String> locals = new HashMap<String, String>();
HashSet<References> refs = new HashSet<References>();
Stack<String> errorMessages = new Stack<String>();
/*
To simplify, we force each named entity (constants, code regions, region sets,
parameters and constraints) to have unique names.
*/
String cannedUnique = "All named entities in PERI CSL must have unique names.";

/*
	DATA that tree walker can use to emit code in target language
*/
HashMap params = new HashMap();
HashMap constants = new HashMap();
HashMap region_sets = new HashMap();
HashMap constraints = new HashMap();
Specification specification = null;

/*
	Some Helper methods for "built-in" domain enumerations
*/

// Enumerate/expand range specification
private CSLDomain fillDomain(int type, String min, String max, String step_temp) 
{	
	String step = step_temp;
	CSLDomain data = null;
	if(type == Types.INT)
	{
		if(step_temp.equals(""))
			step="1";
		data = new IntArray();
	} else if(type == Types.FLOAT)
	{
		if(step_temp.equals(""))
			step="1.0";
		data = new FloatArray();
	}
	data.fillValues(min, max, step);
	return data;
}

// Enumerate/expand power range specification
private CSLDomain fillPowerDomain(int type, String min, String max, String base)
{
	CSLDomain data = null;
	if(type == Types.INT)
	{
		data = new IntArray();
	} else if(type == Types.FLOAT)
	{
		data = new FloatArray();
	}
	data.fillPowerValues(min, max, base);
	return data;
}

// Error Formating
private String formatError(String message) 
{
	String stackMessage = "";
	while(!errorMessages.empty())
		stackMessage = errorMessages.pop() + stackMessage;
	message=stackMessage+message; 
	return message;
}
}

/*
	Language rules: 
*/

/*
	prog : 	'search' 'space' variable_name LCPAREN  space_body RCPAREN
*/
prog
scope{
	// Need to refer back to the problem name every now and then, so keep it in this scope
	String problem_name;
}
	: 'search' 'space' name=variable_name {$prog::problem_name=$name.text;} LCPAREN  space_body RCPAREN
	-> ^(SPACE ^(SPACE_NAME variable_name) space_body)
	;
	
/* The body of parameter space consists of the following blocks:
        code region (optional)
        region set (optional)
        parameter declaration (at least one declaration must be provided)
        constraint declarations (optional)
        constraint conjunction / disjunction (optional)
        ordering info (for search hints)
*/

/*
	space_body : constants* code_region* region_set* parameters constraint_decl* grouping* constraint_spec*
*/
space_body 
	: constants* code_region* region_set* parameters constraint_decl* grouping* constraint_spec*
	-> ^(SPACE_BODY constants* code_region* region_set* parameters constraint_decl* grouping* constraint_spec*)
	;

/*
	constants : 'constants' LCPAREN constant_decl+ RCPAREN
*/

constants
@init{
	errorMessages.push("Parsing given constant declarations:: ");
}
@after{
	errorMessages.pop();
}
	: 'constants' LCPAREN constant_decl+ RCPAREN
	-> ^(CONSTANTS constant_decl+)
	;
	
/*
	constant_decl : param_type variable_name ASSIGN d_vals SEMI
*/
constant_decl
	: (const_type=param_type variable_name ASSIGN d_vals[$param_type.type] SEMI)
	{
		if(constants.get($variable_name.text) != null)
		{
			String msg = "Duplicate Constant Assignment Found: " + $variable_name.text + " is already defined. ";
			msg += "\n" + cannedUnique;
			throw new RuntimeException(msg);
		} 
		else if($param_type.type == Types.MIXED) 
		{
			String msg = "Invalid type (mixed) found for constant " + $variable_name.text + ". ";
			throw new RuntimeException(msg);
		} 
		else 
		{
			Symbol symbol=new Symbol($variable_name.text, $const_type.type, $d_vals.text);
			constants.put($variable_name.text, symbol);
			locals.put($variable_name.text, "constant");
		}
	}
	-> ^(CONST_ASSIGN param_type variable_name d_vals)
	;

/*
	code_region : 'code_region' variable_name SEMI
*/
// begins with 'code_region' keyword
code_region
	: 'code_region' variable_name SEMI
	{
		if(locals.get($variable_name.text)!=null) 
		{
			String msg = "Error :: Code Region Name " + $variable_name.text 
				+ " is already used to define a " + locals.get($variable_name.text) + "\n";
			msg += cannedUnique;
			throw new RuntimeException(msg);
		} 
		else 
		{
			code_regions.add($variable_name.text);
			locals.put($variable_name.text, "code region");
		}
	}
	-> ^(CODE_REGION variable_name)
	;

// As the specification from PERI-Transform group is finalized on how naming for different code and
//  regions is done, this rule will be changed to reflect the specification. For now, this is 
//  duplicates the variable_name rule
/*
	region_name : IDENT
*/
region_name
	:  IDENT
	;

/*
	region_set : 'region_set' region_set_name LSPAREN region_name_list RSPAREN SEMI
*/
region_set
@init {
	errorMessages.push("In Region Set Declaration:: ");
}
@after {
	errorMessages.pop();
}
	:  'region_set' name=region_set_name LSPAREN ls=region_name_list RSPAREN SEMI
	{
		if(locals.get($name.text) != null) 
		{
			String msg = "Error :: Region set Name " + $name.text 
				+ " is used previously to define a " + locals.get($name.text) + "\n";
			msg += cannedUnique; 
			throw new RuntimeException(msg);
		} 
		else 
		{
			List temp=$ls.names;
			Vector string_names = new Vector();
               		// make sure all the code_regions are already defined
	                for(int i=0; i < temp.size(); i++) {
                	        if(!code_regions.contains(((CommonTree)temp.get(i)).getText())) 
                	        {
                        	        String msg = "Error :: code region " + temp.get(i) + " is not defined.";
                                	throw new RuntimeException(msg);
                        	}
                        	else 
                        	{
                        		string_names.add(((CommonTree)temp.get(i)).getText());
                        	}
                	}
        	        RegionSet rs = new RegionSet($name.text);
                	rs.pushList(string_names);
	                region_sets.put($name.text, rs);
	                locals.put($name.text, "region set");
	         }
	  }
	-> ^(REGION_SET ^(REGION_SET_NAME region_set_name) region_name_list)
	;

/*
	region_name_list : region_name (COMMA region_name)*
*/
// COMMA separated names
region_name_list returns [List names]
        : r_names+=region_name (COMMA  r_names+=region_name)*
        {
                $names=$r_names;

        }
        -> ^(SLIST region_name+)
        ;

// same reasoning as code_region_name
/*
	region_set_name : IDENT
*/
region_set_name
	:  IDENT
	;

/*
	parameters : param_decl+
*/
parameters
	: param_decl+
	-> ^(PARAMETERS param_decl+)
	;

/*
	param_decl : 'parameter' variable_name param_type LCPAREN domain_restrictions default_spec? region_spec?
*/
param_decl 
scope{
	String param_name;
}
@init {
	Parameter param = new Parameter();
	errorMessages.push("Inside Parameter Declaration:: ");
}
@after{
	errorMessages.pop();
}
	:  'parameter' name=variable_name 
			{
				// Check to make sure that there are no locals defined with the same name
				//  All named Peri CSL entities *must* have unique names
				$param_decl::param_name=$name.text;
				param.setName($name.text);
				param.setProblemName($prog::problem_name);
			}
			type=param_type 
			{
				param.setType($param_type.type);
			}
			LCPAREN
			(domain_restrictions[$param_type.type]
			{
				param.setDomain($domain_restrictions.data);
			}
			)
			(default_spec[$param_type.type]?
			{
				param.setDefault($default_spec.def);
			}
			)
			(region_spec?
			{
				param.setRegionSet($region_spec.rs);
			})
			RCPAREN
	{
		// put this parameter definition in the HashMap
		params.put($name.text, param);
		locals.put($name.text, "parameter ");
		
	}
	-> ^(PARAMETER_DECL variable_name param_type domain_restrictions default_spec? region_spec?)
	;
/*
	variable_name : IDENT
*/

variable_name
	: IDENT
	;

/*
	param_type : 'int' | 'float' | 'string' | 'bool' | 'mixed'
*/
param_type returns [int type]
	: 'int' 
	{
		$type=Types.INT;
	}
	-> ^(TYPE 'int')
	| 'float' 
	{
		$type=Types.FLOAT;
	}
	-> ^(TYPE 'float')
	| 'bool' 
	{
		$type=Types.BOOLEAN; 
	}
	-> ^(TYPE 'bool')
	| 'string' 
	{
		$type=Types.STRING;
	}
	-> ^(TYPE 'string')
	| 'mixed' 
	{
		$type=Types.MIXED;
		
	} 
	-> ^(TYPE 'mixed')
	;	

/*
	array : d_vals (COMMA d_vals)*
*/
array [int type]	
	: (d_vals[$type]) 
	   (COMMA d_vals[$type])* 
	-> { $type == Types.INT }? ^(INTARR d_vals+)
	-> { $type == Types.FLOAT }? ^(FLOATARR d_vals+)
	-> { $type == Types.STRING }? ^(STRARR d_vals+)
	-> { $type == Types.BOOLEAN }? ^(BOOLARR d_vals+)
	-> { $type == Types.MIXED }? ^(MIXEDARR d_vals+)
	-> ^(ARR d_vals+)
	;

/*
	domain_restrictions : 'range' LSPAREN d_vals COLON d_vals (COLON d_vals)? RSPAREN SEMI
			    | 'prange' LSPAREN d_vals COLON d_vals COLON d_vals RSPAREN SEMI
			    |  'array' LSPAREN array (COMMA LSPAREN array RSPAREN)* RSPAREN SEMI
*/
domain_restrictions [int type] returns [CSLDomain data]
@init {
	int stepDefined = 0;
	Vector accum = new Vector();
	int arrOfArr = 0;
	String temp = "Analyzing domain values for parameter " + $param_decl::param_name + ". ";
	temp += "The parameter is of type: " + Types.getType($type) + ". ";
	errorMessages.push(temp);
}
@after
{
	errorMessages.pop();
}
	: 'range' LSPAREN min=d_vals[$type] COLON max=d_vals[$type] (COLON step=d_vals[$type] { stepDefined=1;} )? RSPAREN SEMI
	{
		if(stepDefined == 0) 
		{ 
			System.out.println("Step is not defined in this case");
			$data = fillDomain($type, $min.text, $max.text, new String(""));
		}
		else
			$data = fillDomain($type, $min.text, $max.text, $step.text);
		
	}
	-> { $type == Types.INT && stepDefined == 1 }? ^(INTRANGE $min $max $step)
	-> { $type == Types.INT && stepDefined == 0 }? ^(INTRANGE $min $max)
	-> { $type == Types.FLOAT && stepDefined == 1 }? ^(FLOATRANGE $min $max $step)
	-> { $type == Types.FLOAT && stepDefined == 0 }? ^(FLOATRANGE $min $max)
	-> ^(RANGE $min $max $step)
	
	| 'prange' LSPAREN min=d_vals[$type] COLON max=d_vals[$type] COLON base=d_vals[$type] RSPAREN SEMI
	{
		$data = fillPowerDomain($type, $min.text, $max.text, $base.text); 
	}
	-> { $type == Types.INT }? ^(INTPRANGE $min $max $base)
	-> { $type == Types.FLOAT }? ^(FLOATPRANGE $min $max $base)
	-> ^(PRANGE $min $max $base)
	
	| 'array' LSPAREN array[$type] (COMMA LSPAREN array[$type] {arrOfArr=1;} RSPAREN)* RSPAREN SEMI
	-> { arrOfArr == 1}? ^(ARROFARR array+)
	-> ^(FLATARR array)
	;

/*
	d_vals : INTEGER | FLOAT | STRING_LITERAL | BOOLEAN
*/
d_vals [int type]
	: INTEGER 
	{
		if(($type != Types.INT) && ($type != Types.MIXED)) 
		{
			String msg="";
			msg += "Type mismatch occurred. Expecting " +  Types.getType($type) 
				+ ", found integer.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	| FLOAT
	{
		if(($type != Types.FLOAT) && ($type != Types.MIXED)) 
		{
			String msg = "";
			msg += "Type mismatch occurred. Expecting " +  Types.getType($type) 
				+ ", found float.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	| STRING_LITERAL 
	{
		if(($type != Types.STRING) && ($type != Types.MIXED)) 
		{
			String msg = "";
			msg += "Type mismatch occurred. Expecting " +  Types.getType($type) 
				+ ", found string.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	| BOOLEAN  
	{
		if(($type != Types.BOOLEAN) && ($type != Types.MIXED)) 
		{
			String msg = "";
			msg += "Type mismatch occurred. Expecting " +  Types.getType($type) 
				+ ", found boolean.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	;

/*
	default_spec : 'default' (INTEGER|FLOAT|STRING|BOOLEAN)
*/
default_spec[int type] returns [DefaultVal def]
	: 'default' val=INTEGER SEMI 
	{
		if($type == Types.INT)
		{
			DefaultVal temp = new DefaultVal(Types.INT, $val.text);
			$def = temp;
		} 
		else 
		{
			String msg = "Error :: Type mismatch. Parameter " + 
				$param_decl::param_name + " is not declared as of type integer.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(DEFAULT INTEGER)
	
	| 'default' val=FLOAT SEMI 
	{
		if($type == Types.FLOAT) 
		{
			DefaultVal temp = new DefaultVal(Types.FLOAT, $val.text);
			$def = temp;
		} 
		else 
		{
			String msg = "Error :: Type mismatch. Parameter " + 
				$param_decl::param_name + " is not declared as of type float.";
			throw new RuntimeException(msg);
		}
	}
	-> ^(DEFAULT FLOAT)
	
	| 'default' val=BOOLEAN SEMI
	{
		if($type == Types.BOOLEAN) 
		{
			DefaultVal temp = new DefaultVal(Types.BOOLEAN, $val.text);
			$def = temp;
		}
		else
		{
			String msg = "Error :: Type mismatch. Parameter " + 
				$param_decl::param_name + " is not declared as of type boolean.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(DEFAULT BOOLEAN)
	
	| 'default' val=STRING_LITERAL SEMI
	{
		if($type == Types.STRING) 
		{
			DefaultVal temp = new DefaultVal(Types.BOOLEAN, $val.text);
			$def = temp;
		}
		else 
		{
			String msg = "Error :: Type mismatch. Parameter " + 
				$param_decl::param_name + " is not declared as of type string.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(DEFAULT $val)
	;

/*
	region_spec : 'region' region_set_name
*/
region_spec returns [RegionSet rs]

	:'region' set_name=region_set_name SEMI
	{
		if(region_sets.get($set_name.text)!=null)
		{
			RegionSet rset = (RegionSet)region_sets.get($set_name.text);
			$rs=rset;
		} 
		else 
		{
			String msg = "Error :: Region Set " + $set_name.text + " is not defined";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
		
	}
	-> ^(REGION_SPEC region_set_name)
	;

/*
	constraint_decl : 'constraint' constraint_name LCPAREN expr SEMI RCPAREN	
*/
constraint_decl
@init {
	errorMessages.push("Parsing Constraint ");
}
@after {
	errorMessages = new Stack<String>();
}	

	:  'constraint' name=constraint_name LCPAREN e=expr SEMI RCPAREN
	{
		Constraint temp = new Constraint($name.text);
		temp.setProblemName($prog::problem_name);
		temp.setArguments(refs);
		constraints.put($name.text, temp);
		locals.put($name.text, "constraint ");
		refs = new HashSet<References>();
	}
	-> ^(CONSTRAINT constraint_name ^(EXPR expr))
	;

/*
	constraint_name : IDENT
*/
constraint_name
@after {
	errorMessages.push($name.text);
}
	:  name=IDENT
	-> ^(CONSTRAINT_NAME IDENT)
	;

/*
	grouping and ordering are not implemented yet. These constructs provide search hints and as such
	are not of much interest for current purpose of representing (and enumerating) points in the
	search space.
*/
grouping
	: 'groups' LCPAREN set_decl* RCPAREN
	;
	
set_decl
	: 'set' LSPAREN args+=param_ref (COMMA args+=param_ref)* SEMI RSPAREN  
	;

/*
	constraint_spec : 'specification' LCPAREN s_expr SEMI RCPAREN
*/
constraint_spec
	:  'specification' LCPAREN s_expr SEMI RCPAREN
	{
		ArrayList<Constraint> spec_cons = new ArrayList<Constraint>();
		Iterator refs_iter = refs.iterator();
		while(refs_iter.hasNext()) 
		{
			String ref_name = refs_iter.next().toString();
			if(constraints.get(ref_name) == null)
			{
				String msg = "ERROR:: Constraint " + ref_name + " referenced in the specification is not defined.";
				throw new RuntimeException(msg);
			}
			spec_cons.add((Constraint)constraints.get(ref_name));
		}
		specification = new Specification(spec_cons);
		specification.setProblemName($prog::problem_name);
	}
	-> ^(SPECIFICATION s_expr)
	
	;
	
s_expr 	:
	primary_spec ((AND|OR)^ primary_spec)*
	;
	
primary_spec
	: LPAREN s_expr RPAREN
	-> ^(GROUPED_SPEC s_expr)
	| spec_c
	{
		refs.add(new ParamReference($spec_c.text));
	}
	-> ^(CONJ spec_c)
	;

spec_c
	: IDENT;	

/*
	EXPRESSIONS	
*/

expr
scope {
	String descr;
}
	:  expr_text = logical_expression
	{
		$expr::descr = $expr_text.text;

	}
	;
		
logical_expression
	:  boolean_and_expression (OR^ boolean_and_expression )*
	;
	
boolean_and_expression
	:  equality_expression (AND^ equality_expression)*
	;

equality_expression
	:  relational_expression ((EQUALS|NOTEQUALS)^ relational_expression)*
	;

relational_expression
	:  additive_expression ((LT|LTEQ|GT|GTEQ)^ additive_expression )*
	;

additive_expression
	:  multiplicative_expression ((PLUS|MINUS)^ multiplicative_expression )*
	;

multiplicative_expression 
	:  power_expression ((MULT|DIV|MOD)^ power_expression )*
	;
	
power_expression 
	:  unary_expression (POW^ unary_expression )*
	; 
	
unary_expression
	:  primary_expression
    	|  NOT^ primary_expression
    	|  MINUS primary_expression -> ^(NEGATE primary_expression)
   	;
  
primary_expression
	:  LPAREN! logical_expression RPAREN!
	|  value 
	;

value returns [Object val]
	: given_val=(INTEGER|FLOAT|BOOLEAN|STRING_LITERAL)
	{ 
		$val=$given_val.text;
	}
	| param_ref
	{
		$val=$param_ref.p_val;
	}
	;
	
/*
	Parameter References
*/
param_ref returns [References p_val]
	: v_name=variable_name
	{
		// make sure the parameter/constant referenced is defined
		if((params.get($v_name.text) !=null) || 
			(constants.get($v_name.text) != null))
		{
			$p_val=new ParamReference($v_name.text);
			if(constants.get($v_name.text) == null)
				refs.add($p_val);
		} 
		else 
		{
			String msg = " Parameter/Constant/Constraint "  +
				$v_name.text + " is not defined.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(PARAM_REF variable_name)
	
	| r_name=region_name DOT v_name=variable_name
	{
		
		Parameter param = (Parameter)params.get($v_name.text);
		Vector code_regions = param.getCodeRegions();
		
		if(code_regions.contains($r_name.text) && (params.get($v_name.text) !=null)) 
		{
			$p_val=new RegionReference($r_name.text, $v_name.text);
			refs.add($p_val);
		} 
		else 
		{
			String msg = "";
			if(!code_regions.contains($r_name.text)) 
			{
				msg+="Code Region " + $r_name.text + " is not associated with parameter " 
				+ $v_name.text;
			} 
			else 
			{
				msg+="Parameter " + $v_name.text + " referenced in " + " is not defined.";
			}
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(REGION_REF region_name variable_name)
	
	| v_name=variable_name DOT VALUE
	{
		if(params.get($v_name.text) !=null) 
		{
			$p_val=new ParamReference($v_name.text);
			refs.add($p_val);
		}
		else 
		{
			String msg = "Parameter/Constant " + $v_name.text + " is not defined.";
			String errorString = formatError(msg);
			throw new RuntimeException(errorString);
		}
	}
	-> ^(ARR_ENUM_REF variable_name)
	;
	
parameter_space_name
	:  IDENT
	;

/* 
	lexer rules start here
*/
VALUE	
	: 'value'
	;
		
STRING_LITERAL
    :  '"' ( 'a'..'z' | 'A'..'Z' | '_' | '0'..'9' | ' ' )* '"'
    ;

UNDERSCORE
	: '_'
	;

EscapeSequence
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    ;

ASSIGN	: '='
	;


OR 	
	: '||' 
	| 'or'
	;

AND 	
	: '&&' 
	| 'and'
	;

EQUALS	
	: '=='
	;

NOTEQUALS 
	: '!=' 
	| '<>'
	;
	
LT	
	: '<'
	;
LTEQ	
	: '<='
	;
GT	
	: '>'
	;
GTEQ	
	:  '>='
	;

PLUS	
	:  '+'
	;
MINUS	
	:  '-'
	;

MULT	
	:  '*'
	;
DIV	
	:  '/'
	;
MOD	
	:  '%'
	;

POW	
	:  '^'
	;

NOT	
	:  '!' 
	|  'not'
	;

DOT	
	: '.'
	;

SEMI
	: ';'
	;

LPAREN 	
	:  '('
	;
	
RPAREN
	:  ')'
	;

COLON
	:  ':'
	;
	
LCPAREN
	: '{'
	;
	
RCPAREN
	: '}'
	;
	
LSPAREN
	: '['
	;
	
RSPAREN
	: ']'
	;	

COMMA 	
	:  ','
	;

INTEGER	
	:  (PLUS|MINUS)? ('0'..'9')+ 
	;

FLOAT
	:   (PLUS|MINUS)? ('0'..'9')* '.' ('0'..'9')+
	;

BOOLEAN
	:  'true'
	|  'false'
	;

IDENT
	:  ('a'..'z' | 'A'..'Z' | '_') ('a'..'z' | 'A'..'Z' | '_' | '0'..'9')*
	;

WS	
	:  (' '|'\r'|'\t'|'\u000C'|'\n') {skip();}
	;
SL_COMMENT
		:	'#'
			(~('\n'|'\r'))* ('\n'|'\r'('\n')?)
			{skip();}
		;
