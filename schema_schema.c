
/* Generated automatically by schema */

/* Private Data */

static short dAttributeOffset [7][13] = {
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,   0,   4,   8,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,   0,   4,   8,  12,  16,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   4,   8,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,   0,   4,   8,  -1,  -1,  -1,  -1,  -1,  12, 
};

static short dAttributeTags [7][13] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   1,   3,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   3,   3,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   3,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
};

static short dClassIsA [7][7] = {
	1, 0, 0, 0, 0, 0, 0, 
	0, 1, 0, 0, 0, 0, 0, 
	0, 1, 1, 0, 0, 0, 0, 
	0, 1, 0, 1, 0, 0, 0, 
	0, 1, 0, 0, 1, 0, 0, 
	0, 1, 0, 0, 0, 1, 0, 
	0, 1, 0, 0, 0, 0, 1, 
};


/* Public Data */

short dObjectSize [7] = {
	0, 0, 12, 20, 12, 0, 16, 
};

int   dObjects = 7;
char *dObjectName [7] = {
	"nINVALID",
	"Object",
	"nSchema",
	"nClass",
	"nAttr",
	"nConstraint",
	"nAttrSym",
};

int   dAttributes = 13;
char *dAttributeName [13] = {
	"qINVALID",
	"qClassTree",
	"qAttrSyms",
	"qAllClasses",
	"qIdent",
	"qText",
	"qCode",
	"qAttrs",
	"qDerived",
	"qAttrSym",
	"qConstraint",
	"qTags",
	"qType",
};

short dAttributeType [13] = {
	  0, 150, 150, 150,   4,  10,   4, 150, 150, 100, 100,   4,   4, 
};


/* Public Functions */

short dGetAttributeOffset (class, attribute)      
short                      class;                 
short                      attribute;             
{                                                 
    return (dAttributeOffset[class][attribute]);  
}                                                 
                                                  
short dGetAttributeType (attribute)               
short                    attribute;               
{                                                 
    return (dAttributeType[attribute]);           
}                                                 
                                                  
short dGetAttributeTags (class, attribute)        
short                    class;                   
short                    attribute;               
{                                                 
    return (dAttributeTags[class][attribute]);    
}                                                 
                                                  
int   dGetClassIsA      (class, isaclass)         
short                    class;                   
short                    isaclass;                
{                                                 
    return (dClassIsA[class][isaclass]);          
}                                                 
                                                  
