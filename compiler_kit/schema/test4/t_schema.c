
/* Generated automatically by schema */

/* Private Data */

static short dAttributeOffset [5][3] = {
	 -1,  -1,  -1, 
	 -1,  -1,  -1, 
	 -1,   0,  -1, 
	 -1,   4,   8, 
	 -1,  -1,   0, 
};

static short dAttributeType [3] = {
	  0, 100, 100, 
};

static short dAttributeTags [5][3] = {
	  0,   0,   0, 
	  0,   0,   0, 
	  0,   0,   0, 
	  0,   0,   0, 
	  0,   0,   0, 
};

static short dClassIsA [5][5] = {
	1, 0, 0, 0, 0, 
	0, 1, 0, 0, 0, 
	0, 1, 1, 0, 0, 
	0, 1, 1, 1, 0, 
	0, 1, 0, 0, 1, 
};


/* Public Data */

short dObjectSize [5] = {
	0, 0, 4, 12, 4, 
};

int   dObjects = 5;
char *dObjectName [5] = {
	"nINVALID",
	"Object",
	"AddressedObject",
	"FuncDef",
	"XObject",
};

int   dAttributes = 3;
char *dAttributeName [3] = {
	"qINVALID",
	"qAddr",
	"qBody",
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
                                                  
