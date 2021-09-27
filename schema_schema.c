
/* Generated automatically by schema */

/* Private Data */

static short dAttributeOffset [7][11] = {
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,   0,   8,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,   0,   8,  16,  24,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   8,  16,  24, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,   0,   8,  -1,  -1,  -1,  -1,  -1,  -1, 
};

static short dAttributeType [7][11] = {
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1, 100, 150,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,   4,   4, 150, 150,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1, 100,   4, 100,   4, 
	 -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 
	 -1,  -1,  -1,   4,   4,  -1,  -1,  -1,  -1,  -1,  -1, 
};

static short dAttributeTags [7][11] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   1,   3,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   3,   3,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   4,   0,   3,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
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
	0, 0, 16, 32, 32, 0, 16, 
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

int   dAttributes = 11;
char *dAttributeName [11] = {
	"qINVALID",
	"qClassTree",
	"qAttrSyms",
	"qIdent",
	"qCode",
	"qAttrs",
	"qDerived",
	"qAttrSym",
	"qType",
	"qConstraint",
	"qTags",
};


/* Public Functions */

short dGetAttributeOffset (class, attribute)      
short                      class;                 
short                      attribute;             
{                                                 
    return (dAttributeOffset[class][attribute]);  
}                                                 
                                                  
short dGetAttributeType (class, attribute)        
short                    class;                   
short                    attribute;               
{                                                 
    return (dAttributeType[class][attribute]);    
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
                                                  
