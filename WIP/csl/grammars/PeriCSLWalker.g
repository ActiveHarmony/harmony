tree grammar PeriCSLWalker;

options {
    // use the vocabulary from the parser
    tokenVocab=PeriCSL;
    ASTLabelType=CommonTree;
    // generate templates
    output=template; 
}

@header {
package org.umd.periCSL;
// imports
import java.util.HashMap;
import java.util.Vector;
import java.util.HashSet;
}

@members {
/** Points at locals filled in by the parser */
HashMap params;
HashMap region_sets;
HashMap constants;
HashMap constraints;
Specification specification;
}
// END:header

prog[HashMap params, HashMap region_sets, 
	HashMap constants, HashMap constraints, 
	Specification specification]
scope {
	String problem_name;
}
@init {
this.params = params;
this.region_sets = region_sets;
this.constants=constants;
this.constraints = constraints;
this.specification = specification;
}
    : ^(SPACE 
    	^(SPACE_NAME id=IDENT) 
    	sb=space_body[$id.text]) 
    	{
    		$prog::problem_name=$id.text;
    	}
    -> start(name={$id.text}, body={$sb.st})
    ;
  
space_body [String pname]
	: ^(SPACE_BODY 
	  (^(CONSTANTS cons+=constant_decl+))*
	  codeReg+=code_region* 
	  regSet+=region_set* 
	  ^(PARAMETERS pDecls+=parameters+) 
	  consDecls += constraint_decl*
	  specDecl += constraint_spec?
	  )
	-> body(pname={$pname}, constDecls={$cons}, codeRegions={$codeReg}, 
		regionSets={$regSet}, paramDecls={$pDecls}, 
		constraintDecls={$consDecls}, 
		specDecl={$specDecl})
	;

constant_decl
	: ^(CONST_ASSIGN 
		type name=IDENT 
		val=d_vals)
	-> cDecl(type={$type.st}, name={$name.text}, 
		val={$val.st}) 
	;
	
code_region
	: ^(CODE_REGION name=IDENT)
	-> codeRegion(name={$name.text})
	;


region_set
	: ^(REGION_SET 
		^(REGION_SET_NAME name=IDENT) 
		^(SLIST code_reg_reffed+=IDENT+))
	-> regionSet(name={$name.text}, lst={$code_reg_reffed})
	;
	
parameters
@init {
	Parameter param = null;
	Boolean builtIn = new Boolean("false");
}
	
	: ^(PARAMETER_DECL 
		name=IDENT 
		type 
		domain
		ds= default_spec[$type.type]?
		rs= region_spec?
		
	    )
	{
		param=(Parameter)params.get($name.text);
	}
	/* Both range and prange (power range) are considered to be built-in functions.
	 *   We simply grab the applicable domain values from the Parameter Object.
	 *   Parameter Object can be grabbed from the HashMap.
	 * If you would rather get the min, max and step/base numbers, set the builtIn
	 *   flag to 0.  
	 */
	//-> { (builtIn == 1) && ($domain.range == 1)}? pDeclRange(parameter={param},type={$type.st},def={$ds.st},regSpec={$rs.st})
	-> pDecl(parameter={param},type={$type.st}, domain={$domain.st},def={$ds.st},regSpec={$rs.st}, builtIn={builtIn})
	;

type returns [int type]
	: ^(TYPE (
		  t='int' 
		  { $type=Types.INT; }
		-> intTypeDecl(type={$t.text})
		
		| t='float' 
		{ $type=Types.FLOAT; }
		-> floatTypeDecl(type={$t.text})
		
		| t='bool' 
		{ $type=Types.BOOLEAN; }
		-> boolTypeDecl(type={$t.text})
		
		| t='string' 
		{ $type=Types.STRING; }
		-> stringTypeDecl(type={$t.text})
		
		| t='mixed'  
		{ $type=Types.MIXED; }
		-> mixedTypeDecl(type={$t.text})
		)
		)
	;
	
domain returns [int range]
	:   
	      ^(INTPRANGE min=INTEGER max=INTEGER (base=INTEGER)?) { $range = 1;}
	      -> rangePInt(min={$min}, max={$max}, base={$base})
	      
	    | ^(INTRANGE min=INTEGER max=INTEGER (step=INTEGER)?) { $range = 1;}
	      -> rangeInt(min={$min}, max={$max}, step={$step})
	      
	    | ^(FLOATPRANGE min=FLOAT max=FLOAT (base=FLOAT)?) { $range = 1;}
	      -> rangePFloat(min={$min}, max={$max}, base={$base})
	      
	    | ^(FLOATRANGE min=FLOAT max=FLOAT (step=FLOAT)?) { $range = 1;}
	      -> rangeFloat(min={$min}, max={$max}, step={$step})
	      
	    | ^(FLATARR array)
	      -> {$array.st}
	      
	    | ^(ARROFARR arr+=array+)
	      -> arr(vals={$arr})
	    
	;
	
d_vals
@init {
	int trueOrFalse = 0;
}
	: 
	  INTEGER -> intVal(integer={$INTEGER.text})
	  
	| FLOAT -> floatVal(float={$FLOAT.text})
	
	| STRING_LITERAL -> strVal(str={$STRING_LITERAL.text})
	
	| val=BOOLEAN
	{
		if(($val.text).equals("true"))
			trueOrFalse=1;
	}
	-> { trueOrFalse == 1}? boolTVal(bool={$BOOLEAN.text})//{%{$BOOLEAN.text}}
	-> boolFVal(bool={$BOOLEAN.text})
	;
	
array 	
	: 
	(
	  ^(INTARR vals+=d_vals+)
	| ^(FLOATARR vals+=d_vals+)
	| ^(STRARR vals+=d_vals+)
	| ^(BOOLARR vals+=d_vals+)
	| ^(MIXEDARR vals+= d_vals+)
	| ^(ARR vals+=d_vals+)
	)
	 -> arr(vals={$vals})
	;

default_spec[int type]
	: ^(DEFAULT val=d_vals)
	-> defDecl(type={$type}, val={$val.st})
	;

region_spec
	: ^(REGION_SPEC IDENT)
	-> regionSpec(name={$IDENT.text})
	;
	
constraint_decl
@init {
	Constraint constrObject = null;
	
}
	: ^(CONSTRAINT ^(CONSTRAINT_NAME name=IDENT) ^(EXPR e=expr))
	{
		constrObject = (Constraint)constraints.get($name.text);
		//arguments= temp.getArguments();
	}
	// Check if specification in provided
	-> constraint(constrObject={constrObject}, ex={$expr.st}, spec={specification})
	;
	
expr
	:  d_vals
	-> {$d_vals.st}
	
	| param_ref
	-> {$param_ref.st}
	
	|  ^(OR expr1=expr expr2=expr)
	-> orExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(AND expr1=expr expr2=expr)
	-> andExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(EQUALS expr1=expr expr2=expr)
	-> equalsExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(NOTEQUALS expr1=expr expr2=expr)
	-> notEqualsExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(LT expr1=expr expr2=expr)
	-> lessThanExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(LTEQ expr1=expr expr2=expr)
	-> lessThanEqualsExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(GT expr1=expr expr2=expr)
	-> greaterThanExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(GTEQ expr1=expr expr2=expr)
	-> greaterThanEqualsExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(PLUS expr1=expr expr2=expr)
	-> plusExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(MINUS expr1=expr expr2=expr)
	-> minusExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(MULT expr1=expr expr2=expr)
	-> multExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(DIV expr1=expr expr2=expr)
	-> divExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(MOD expr1=expr expr2=expr)
	-> modExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	| ^(POW expr1=expr expr2=expr)
	-> powExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	| ^(NOT expr1=expr)
	-> notExpr(expr1={$expr1.st})
	
	| ^(NEGATE expr1=expr)
	-> negateExpr(expr1={$expr1.st})
	;

param_ref returns [Vector<References> refs]
@init {
	$refs= new Vector<References>();
}


	: ^(PARAM_REF IDENT)
	{
		$refs.add(new ParamReference($IDENT.text));
		
	}
	-> paramRef(name={$IDENT})
	
	| ^(REGION_REF region=IDENT param=IDENT)
	{
		$refs.add(new RegionReference($region.text, $param.text));
	}
	-> regionRef(region={$region.text}, param={$param.text})
	
	| ^(ARR_ENUM_REF param=IDENT)
	{
		$refs.add(new ParamReference($IDENT.text));
	}
	-> arrRef(name={$param.text})
	;

constraint_spec
	:  ^(SPECIFICATION s_expr)
	-> specification(spec={specification}, expr={$s_expr.st})
	;
		
s_expr 	
@init {
	Constraint c = null;
}
	:  ^(OR expr1=s_expr expr2=s_expr)
	-> specOrExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	|  ^(AND expr1=s_expr expr2=s_expr)
	-> specAndExpr(expr1={$expr1.st}, expr2={$expr2.st})
	
	| ^(GROUPED_SPEC e=s_expr) -> {e.st}
	
	| ^(CONJ spec_c)
	{
		c=(Constraint)constraints.get($spec_c.text);
	}
	->constraintCall(c={c})
	;
	
primary_spec
@init {
	Constraint c = null;
}
	: ^(GROUPED_SPEC e=s_expr) 
	-> {e.st}
	
	| ^(CONJ spec_c)
	{
		c=(Constraint)constraints.get($spec_c.text);
	}
	->constraintCall(c={c})
	;

spec_c
	: IDENT;	
