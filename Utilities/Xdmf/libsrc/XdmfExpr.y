%{
/* Force the definition for Linux */
/* Possible bug in older Linux yacc */
#ifndef NOBISON
extern int yylex();
extern "C" {
	void yyerror( char *);
	int  yyparse( void );
}
#endif
#include <XdmfExpr.h>
#include <XdmfArray.h>
#include <XdmfHDF.h>
#include <math.h>

static XdmfArray *XdmfExprReturnValue;

class XdmfInt64Array : public XdmfArray {
public :
	XdmfInt64Array( XdmfInt64 Length ) {
		this->SetNumberType( XDMF_INT64_TYPE );
		this->SetNumberOfElements( Length );
		}
	XdmfInt64Array() {
		this->SetNumberType( XDMF_INT64_TYPE );
		this->SetNumberOfElements( 10 );
		};
};

#define ADD_XDMF_ARRAY_TO_SYMBOL( a ) \
	{ \
	char	name[80]; \
	XdmfExprSymbol *sp; \
	sprintf( name, "XdmfArray_%X", ( XdmfLength)(a) ); \
	sp = XdmfExprSymbolLookup( name ); \
	sp->ClientData = (a); \
	}

%}

%union {
	double		DoubleValue;
	long		IntegerValue;
	void		*ArrayPointer;
	XdmfExprSymbol	*Symbol;
}

%token <DoubleValue>	FLOAT
%token <IntegerValue>	INTEGER
%token <ArrayPointer>	ARRAY
%token <Symbol>		NAME

%token SIN COS TAN ACOS ASIN ATAN LOG EXP ABS_TOKEN SQRT WHERE
%token EQEQ LT LE GT GE NE LTLT GTGT JOIN

%left	'-' '+'
%left	'*' '/'
%left	',' ';'

%type <DoubleValue> ScalarExpression
%type <ArrayPointer> ArrayExpression
%%

/* Comments */
statemant_list : statement {
		/* 
		printf("Complete\n");
		printf("XdmfExprReturnValue Nelms = %d\n", XdmfExprReturnValue->GetNumberOfElements());
		*/
		}
	;

statement: ARRAY '=' ArrayExpression	{
		XdmfArray *TempArray = ( XdmfArray *)$3;

		/* printf("Setting %s from ArrayExpression\n", $1); */
		XdmfExprReturnValue = (XdmfArray *)$1;
		*XdmfExprReturnValue = *TempArray;
		delete TempArray;
		}
	|  ARRAY '=' ScalarExpression {
		/* printf("Setting %s from ScalarExpression\n", $1); */
		XdmfExprReturnValue = (XdmfArray *)$1;
		*XdmfExprReturnValue = $3;
		}
	|	ARRAY '[' ArrayExpression ']'  '=' ScalarExpression {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfArray	*Result = ( XdmfArray *)$1;
			XdmfLength	i, index, Length = Array1->GetNumberOfElements();

			for( i = 0 ; i < Length ; i++ ){
				index = Array1->GetValueAsFloat64( i );
				Result->SetValueFromFloat64( index, $6 );
				}
			delete Array1;
			XdmfExprReturnValue = Result;
		}
	|	ARRAY '[' ArrayExpression ']'  '=' ArrayExpression {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfArray	*Array2 = ( XdmfArray *)$6;
			XdmfArray	*Result = ( XdmfArray *)$1;
			XdmfFloat64	Value;
			XdmfLength	i, index, Length = Array1->GetNumberOfElements();

			for( i = 0 ; i < Length ; i++ ){
				index = Array1->GetValueAsFloat64( i );
				Value = Array2->GetValueAsFloat64( i );
				Result->SetValueFromFloat64( index, Value );
				}
			delete Array1;
			delete Array2;
			XdmfExprReturnValue = Result;
		}
	|	ARRAY '[' INTEGER ':' INTEGER ']'  '=' ScalarExpression {
			XdmfArray *Range, *Result;

			/* printf("Array Range %d:%d = ScalarExpression \n", $3, $5);	 */
			Range = (XdmfArray *)$1;
			XdmfExprReturnValue = Range->Reference( $3, $5 ); /* This is a Reference */
			*XdmfExprReturnValue = $8;

			/* Now Point to the Entire Array */
			XdmfExprReturnValue = (XdmfArray *)$1;
			}
	|	ARRAY '[' INTEGER ':' INTEGER ']'  '=' ArrayExpression {
			XdmfArray *TempArray = ( XdmfArray *)$8;
			XdmfArray *Range, *Result;

			/* printf("Array Range %d:%d = ArrayExpression \n", $3, $5);	 */
			Range = (XdmfArray *)$1;
			XdmfExprReturnValue = Range->Reference( $3, $5 ); /* This is a Reference */
			*XdmfExprReturnValue = *TempArray;

			/* Now Point to the Entire Array */
			XdmfExprReturnValue = (XdmfArray *)$1;
			delete TempArray;
			}
	|  ArrayExpression {
		XdmfArray *TempArray = ( XdmfArray *)$1;

		/* printf("Clone from ArrayExpression\n"); */
		XdmfExprReturnValue = TempArray;	
		/* printf("XdmfExprReturnValue Nelms = %d\n", XdmfExprReturnValue->GetNumberOfElements()); */
		}
	|  ScalarExpression	{
		printf("Pointless !! Scalar = %g\n", $1);
		}
	;

ArrayExpression: ArrayExpression '+' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;

			/* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
			*Array1 += *Array2;
			$$ = Array1;
			delete Array2;
			}
	|	ArrayExpression ',' ArrayExpression {
			/* Interlace */
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;
			XdmfArray *NewArray = new XdmfArray();
			XdmfInt32 i, Rank1, Rank2;
			XdmfInt64 NewLength, Length1, Length2, IFactor, Lcd;
			XdmfInt64 Dimension1[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Dimension2[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Start[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Stride[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Count[ XDMF_MAX_DIMENSION ];
			XdmfInt64 NewDimension[ XDMF_MAX_DIMENSION ];

			/* printf("Array 0x%X , 0x%X\n", Array1, Array2); */
			
			Rank1 = Array1->GetShape( Dimension1 );
			Rank2 = Array2->GetShape( Dimension2 );
			if( Rank1 != Rank2 ){
				printf(" Interlace : Rank Mismatch !!\n");
				}
			NewArray->CopyType( Array1 );

			Length1 = Array1->GetNumberOfElements();
			Length2 = Array2->GetNumberOfElements();
			NewLength = Length1 + Length2;
			IFactor = Length1 / Length2;
			Lcd = Length1;
			if( Length2 < Length1 ){
				Lcd = Length2;
				}
			NewDimension[0] = Lcd;
			NewDimension[1] = NewLength / Lcd;
			NewArray->SetShape( 2, NewDimension );
			/*
			printf("Rank1 = %d Rank2 = %d\n", Rank1, Rank2 );
			printf("Array1 Size = %d\n", Array1->GetNumberOfElements() );
			printf("Array2 Size = %d\n", Array2->GetNumberOfElements() );
			printf("NewLength = %d\n", NewLength );
			printf("Lcd = %d\n", Lcd );
			printf("IFactor = %d\n", IFactor );
			printf("New Dims = %s\n", NewArray->GetShapeAsString() );
			*/
			/* NewArray->Generate( -55.0,  -55.0 ); */
			/* Copy in Array 1 */
			Start[0] = 0; Start[1] = 0;
			Stride[0] = 1; Stride[1] = 1;
			Count[0] = Lcd; Count[1] = Length1 / Lcd;
			NewArray->SelectHyperSlab( Start, Stride, Count );
			Array1->SelectAll();
			/*
			printf("Copy in Array1 = %s\n", NewArray->GetHyperSlabAsString() );
			*/
			CopyArray( Array1, NewArray );
			/* Copy in Array 2 */
			Start[0] = 0; Start[1] = Length1 / Lcd;
			Stride[0] = 1; Stride[1] = 1;
			Count[0] = Lcd; Count[1] = Length2 / Lcd;
			NewArray->SelectHyperSlab( Start, Stride, Count );
			Array2->SelectAll();
			/*
			printf("Copy in Array2 = %s\n", NewArray->GetHyperSlabAsString() );
			*/
			CopyArray( Array2, NewArray );
			NewDimension[0] = Dimension1[0] + Dimension2[0];
			for( i = 1 ; i < Rank1 ; i++ ){
				NewDimension[i] = Dimension1[i];
				} 
			NewArray->Reform( Rank1, NewDimension );
			/*	
			printf("Result(%s) = %s\n", NewArray->GetShapeAsString(), NewArray->GetValues() );
			*/
			$$ = NewArray;
			delete Array1;
			delete Array2;
			}

	|	ArrayExpression ';' ArrayExpression {
			/* Interlace */
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;
			XdmfArray *NewArray = new XdmfArray();
			XdmfInt32 i, Rank1, Rank2;
			XdmfInt64 Dimension1[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Dimension2[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Start[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Stride[ XDMF_MAX_DIMENSION ];
			XdmfInt64 Count[ XDMF_MAX_DIMENSION ];
			XdmfInt64 NewDimension[ XDMF_MAX_DIMENSION ];

		 	/* printf("Array 0x%X  << 0x%X\n", Array1, Array2); */
			
			Rank1 = Array1->GetShape( Dimension1 );
			Rank2 = Array2->GetShape( Dimension2 );
			if( Rank1 != Rank2 ){
				printf(" Cat : Rank Mismatch !!\n");
				}
			NewDimension[0] = Dimension1[0] + Dimension2[0];
			for( i = 1 ; i < Rank1 ; i++ ){
				NewDimension[i] = Dimension1[i];
				} 
			NewArray->CopyType( Array1 );
			NewArray->SetShape( Rank1, NewDimension );

			/*
			NewArray->Generate( -55.0,  -55.0 );
			*/
			/* Copy in Array 1 */
			for( i = 0 ; i < Rank1 ; i++ ){
				Start[i] = 0;
				Stride[i] = 1;
				Count[i] = Dimension1[i];
				}
			NewArray->SelectHyperSlab( Start, Stride, Count );
			Array1->SelectAll();
			/*
			printf("Copy in Array1 = %s\n", NewArray->GetHyperSlabAsString() );
			*/
			CopyArray( Array1, NewArray );
			/* Copy in Array 2 */
			Start[0] = Dimension1[0];
			Stride[0] = 1;
			Count[0] = Dimension2[0];
			for( i = 1 ; i < Rank1 ; i++ ){
				Start[i] = 0;
				Stride[i] = 1;
				Count[i] = Dimension1[i];
				}
			NewArray->SelectHyperSlab( Start, Stride, Count );
			Array2->SelectAll();
			/*
			printf("Copy in Array2 = %s\n", NewArray->GetHyperSlabAsString() );
			*/
			CopyArray( Array2, NewArray );
			/*
			printf("Result(%s) = %s\n", NewArray->GetShapeAsString(), NewArray->GetValues() );
			*/
			$$ = NewArray;
			delete Array1;
			delete Array2;
			}
	|	ArrayExpression '-' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;

			/* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
			*Array1 -= *Array2;
			$$ = Array1;
			delete Array2;
			}
	|	ArrayExpression '*' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;

			/* printf("Array 0x%X * 0x%X\n", Array1, Array2); */
			*Array1 *= *Array2;
			$$ = Array1;
			delete Array2;
			/* printf("Array1 Nelms = %d\n", Array1->GetNumberOfElements()); */
			}
	|	ArrayExpression '/' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Array2 = ( XdmfArray *)$3;

			/* printf("Array 0x%X + 0x%X\n", Array1, Array2); */
			*Array1 /= *Array2;
			$$ = Array1;
			delete Array2;
			}
	|	ArrayExpression '+' ScalarExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Result;

			/* printf("Array + %g\n", $3); */
			Result  = Array1;
			*Result += $3;
			$$ = Result;
			}
	|	ArrayExpression '-' ScalarExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Result;

			/* printf("Array - %g\n", $3); */
			Result  = Array1;
			*Result -= $3;
			$$ = Result;
			}
	|	ArrayExpression '*' ScalarExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Result;

			/* printf("Array * %g\n", $3); */
			Result  = Array1;
			*Result *= $3;
			$$ = Result;
			}
	|	ArrayExpression '/' ScalarExpression {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Result;

			/* printf("Array / %g\n", $3); */
			Result  = Array1;
			*Result /= $3;
			$$ = Result;
			}
	|	ScalarExpression '+' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$3;
			XdmfArray *Result;

			/* printf("Array + %g\n", $1); */
			Result  = Array1;
			*Result += $1;
			$$ = Result;
			}
	|	ScalarExpression '-' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$3;
			XdmfArray *Result;

			/* printf("Array - %g\n", $1); */
			Result  = Array1;
			*Result -= $1;
			$$ = Result;
			}
	|	ScalarExpression '*' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$3;
			XdmfArray *Result;

			/* printf("Array * %g\n", $1); */
			Result  = Array1;
			*Result *= $1;
			$$ = Result;
			}
	|	ScalarExpression '/' ArrayExpression {
			XdmfArray *Array1 = ( XdmfArray *)$3;
			XdmfArray *Result;

			/* printf("Array / %g\n", $1); */
			Result  = Array1;
			*Result /= $1;
			$$ = Result;
			}
	|	ARRAY '[' ArrayExpression ']' {
			XdmfArray	*Array1 = ( XdmfArray *)$1;
			XdmfArray	*Array2 = ( XdmfArray *)$3;
			XdmfArray	*Result;

			/* printf("ArrayExpression From Indexes\n"); */
			Result = Array1->Clone( Array2 );
			delete Array2;
			$$ = Result;
		}
	|	ARRAY '[' INTEGER ':' INTEGER ']' {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Range, *Result;

			/* printf("ArrayExpression From Array Range %d:%d\n", $3, $5);	 */
			Range = Array1->Reference( $3, $5 ); /* This not a copy  */
	
			Result  = Range->Clone(); /* So Copy It */
			delete Array1;
			$$ = Result;
			}
	|	WHERE '(' ArrayExpression EQEQ ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value == SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	WHERE '(' ArrayExpression LT ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value < SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	WHERE '(' ArrayExpression LE ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value <= SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	WHERE '(' ArrayExpression GT ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value > SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	WHERE '(' ArrayExpression GE ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value >= SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	WHERE '(' ArrayExpression NE ScalarExpression ')' {
			XdmfArray	*Array1 = ( XdmfArray *)$3;
			XdmfLength	i, cntr = 0;
			XdmfLength	Length = Array1->GetNumberOfElements();
			XdmfInt64Array	*Index = new XdmfInt64Array( Length );
			XdmfFloat64	Value, SValue = $5;

			for( i = 0 ; i < Length ; i++ ){
				Value = Array1->GetValueAsFloat64( i );
				if( Value != SValue ) {
					Index->SetValue( cntr++, i );
					}
				}	
			/* printf("Found %d Wheres\n", cntr ); */
			if( cntr == 0 ){
				yyerror("WHERE Function Length == 0");
				return( NULL );
				}
			Index->SetNumberOfElements( cntr );
			$$ = ( XdmfArray *)Index;
			}
	|	NAME '(' ArrayExpression ')' {

			if( $1->DoubleFunctionPtr == NULL ){
				/* printf("Bad Function Ptr for %s\n", $1->Name ); */
				$$ = $3;
			} else {
				XdmfArray *Array1 = ( XdmfArray *)$3;
				XdmfFloat64	Value;
				XdmfLength	i, Length = Array1->GetNumberOfElements();

				/* printf("Function Call %s\n", $1->Name ); */
				for( i = 0 ; i < Length ; i++ ){
					Value = Array1->GetValueAsFloat64( i );
					Array1->SetValueFromFloat64( i, ($1->DoubleFunctionPtr)( Value ) );
					}	
				$$ = Array1;
			}
			}
	|	'(' ArrayExpression ')'	{
			/* printf("( ArrayExpression )\n"); */
			$$ = $2;
			}
	|	JOIN '(' ArrayExpression ')'	{
			/* printf("( ArrayExpression )\n"); */
			$$ = $3;
			}
	|	ARRAY {
			XdmfArray *Array1 = ( XdmfArray *)$1;
			XdmfArray *Result;

			/* printf("ArrayExpression From Array\n"); */

			if ( Array1 == NULL ){
				/* Bomb */
				yyerror("NULL Array Pointer");
				return( NULL );
			} else {
				Result  = Array1->Clone();
				$$ = Result;
				}
			}
	;

ScalarExpression: 	ScalarExpression '+' ScalarExpression {
			/* printf("Scalar +\n"); */
			$$ = $1 + $3;
			}
	|	ScalarExpression '-' ScalarExpression {
			/* printf("Scalar -\n"); */
			$$ = $1 - $3;
			}
	|	ScalarExpression '*' ScalarExpression {
			/* printf("Scalar *\n"); */
			$$ = $1 * $3;
			}
	|	ScalarExpression '/' ScalarExpression {
			/* printf("Scalar /\n"); */
			$$ = $1 / $3;
			}
	|	NAME '(' ScalarExpression ')' {
			if( $1->DoubleFunctionPtr == NULL ){
				/* printf("Bad Function Ptr for %s\n", $1->Name ); */
				$$ = 0.0;
			} else {
				$$ = ($1->DoubleFunctionPtr)( $3 );
			}
			}
	|	'(' ScalarExpression ')' {
			/* printf ("( ScalarExpression )\n"); */
			$$ = $2;
			}
	|	INTEGER {
			/* printf ("ScalarExpression from INTEGER\n"); */
			$$ = $1;
			}
	|	FLOAT {
			/* printf ("ScalarExpression from FLOAT\n"); */
			$$ = $1;
			}
	;


%%

/* extern	FILE	*yyin, *yyout; */

#ifdef __cplusplus
/**/
extern "C" {
/**/
#endif

static	char	InputBuffer[ 512 ];
static	int	InputBufferPtr = 0, InputBufferEnd = 0;
static	char	OutputBuffer[ 512 ];
static	int	OutputBufferPtr = 0, OutputBufferEnd = 511;

int
dice_yywrap( void ) {

return 1;
}

void
dice_yyerror( char *string ) {
fprintf(stderr, "XdmfExpr : %s \n", string);
}

int
XdmfExprFlexInput( char *buf, int maxlen ) {
if ( InputBufferPtr < InputBufferEnd ){
	buf[0] = InputBuffer[ InputBufferPtr++ ];
	return(1);
} else {
	buf[0] = '\n';
	return( 0 );
	}
}

int
XdmfExprInput( void ){

if ( InputBufferPtr < InputBufferEnd ){
	return( InputBuffer[ InputBufferPtr++ ] );
} else {
	return '\n';
	}
}

void
XdmfExprUnput( int c ) {
if( InputBufferPtr > 0 ){
	InputBufferPtr--;
	InputBuffer[ InputBufferPtr ] = c;
	}
}

void
XdmfExprOutput( int c ) {
	/* printf("XdmfExprOutput Called\n"); */
	OutputBuffer[ OutputBufferPtr++ ] = c;
	OutputBuffer[ OutputBufferPtr ] = '\0';
	}

XdmfExprSymbol
*XdmfExprSymbolLookup( char *Name ){

static XdmfExprSymbol *Table = NULL;

XdmfExprSymbol	*Last = NULL, *Item = Table;

if( Name == NULL ) {
	/* Table Check  */
	return( Table );
	}

while( Item != NULL ) {
	if( strcmp( Item->Name, Name ) == 0 ) {
		/* printf("Found Symbol %s\n", Name ); */
		return( Item );
		}
	Last = Item;
	Item = Item->Next;
}
/* Not Found : Create New One */
Item = ( XdmfExprSymbol *)calloc( 1, sizeof( XdmfExprSymbol ));
Item->Next = NULL;
Item->Name = strdup( Name );
Item->ClientData = NULL;
Item->DoubleValue = 0;
Item->DoubleFunctionPtr = NULL;
if( Table == NULL ) {
	Table = Item;
	}
if( Last != NULL ){
	Last->Next = Item;
	}
/* printf("New Symbol for %s\n", Name ); */
return( Item );
}

#ifdef __cplusplus
/**/
}
/**/
#endif

XdmfArray *
XdmfExprParse( char *string ){

XdmfExprSymbol	*Item;
XdmfLength	CurrentTime;
XdmfLength	TimeOfCreation;
XdmfArray	*ap;

/* Build the Symbol Table if Necessary */
Item = XdmfExprSymbolLookup( NULL );
if( Item == NULL ){
	/* printf("Creating Symbol Table\n"); */
	Item = XdmfExprSymbolLookup( "cos" );
	Item->DoubleFunctionPtr = cos;
	Item = XdmfExprSymbolLookup( "sin" );
	Item->DoubleFunctionPtr = sin;
	Item = XdmfExprSymbolLookup( "exp" );
	Item->DoubleFunctionPtr = exp;
	Item = XdmfExprSymbolLookup( "tan" );
	Item->DoubleFunctionPtr = tan;
	Item = XdmfExprSymbolLookup( "acos" );
	Item->DoubleFunctionPtr = acos;
	Item = XdmfExprSymbolLookup( "asin" );
	Item->DoubleFunctionPtr = asin;
	Item = XdmfExprSymbolLookup( "atan" );
	Item->DoubleFunctionPtr = atan;
	Item = XdmfExprSymbolLookup( "log" );
	Item->DoubleFunctionPtr = log;
	Item = XdmfExprSymbolLookup( "sqrt" );
	Item->DoubleFunctionPtr = sqrt;
	}
/* Print Symbol Table */
Item = XdmfExprSymbolLookup( NULL );
while( Item != NULL ) {
	if( Item->ClientData != NULL ){
		/* printf("Found Symbol %s\n", Item->Name ); */
		}
	Item = Item->Next;
	}
strcpy( InputBuffer, string );
InputBufferEnd = strlen( InputBuffer );
InputBufferPtr = OutputBufferPtr = 0;
XdmfExprReturnValue = NULL;
/* printf("XdmfExprParse Scanning <%s>\n", InputBuffer); */
CurrentTime = GetCurrentArrayTime();
if ( yyparse() != 0 ){
	/* Error */
	XdmfExprReturnValue = NULL;
	}
/* Remove All Arrays Older than when we started */
/* printf("Cleaning up Temparary Arrays\n"); */
while( ( ap = GetNextOlderArray( CurrentTime, &TimeOfCreation ) ) != NULL ){
	/* Don't remove the return value */
	if( ap != XdmfExprReturnValue ){
		/* printf("Removing Temporary Array\n"); */
		delete ap;
		}
	CurrentTime = TimeOfCreation;
	}
return( XdmfExprReturnValue );
}

