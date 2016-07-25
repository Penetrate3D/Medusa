#ifndef REXCHAR_H
#define REXCHAR_H
 
#define REX_NUMBER	 string("(0|1|2|3|4|5|6|7|8|9)")
#define REX_CHAR 	string("(_|A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p|q|r|s|t|u|v|w|x|y|z)")
#define REX_IF		string("if")			
#define REX_WHILE		string("while")
#define REX_ELSE		string("else")
#define REX_LBRACE		string("{")
#define REX_RBRACE		string("}")
#define REX_CHAR_KEY		string("char")
#define REX_INT		string("int")
#define REX_FLOAT		string("float")	
#define REX_ADD		string("+")
#define REX_DEC		string("-")
#define REX_MUL		string("*")
#define REX_DIV		string("/")
#define REX_LPAREN		string("(")
#define REX_RPAREN		string(")")
#define REX_DOT			string(".")
#define REX_NEWL 		string("\n")
#define REX_WHITE		string("\t| ")
#define REX_OR			string("|")
#define REX_SKEW		string("/")

#define DELETE(x)	do		\
	{                       \
		if(x)				\
			delete x;		\
		x=NULL;				\
	} while (0);


#endif