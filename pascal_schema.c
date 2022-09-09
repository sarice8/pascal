
/* Generated automatically by schema */

/* Private Data */

static short dAttributeOffset [3][2] = {
	 -1,  -1, 
	 -1,  -1, 
	 -1,   0, 
};

static short dAttributeType [3][2] = {
	 -1,  -1, 
	 -1,  -1, 
	 -1, 100, 
};

static short dAttributeTags [3][2] = {
	  0,   0, 
	  0,   0, 
	  0,   6, 
};

static short dClassIsA [3][3] = {
	1, 0, 0, 
	0, 1, 0, 
	0, 1, 1, 
};


/* Public Data */

short dObjectSize [3] = {
	0, 0, 8, 
};

int   dObjects = 3;
char *dObjectName [3] = {
	"nINVALID",
	"Object",
	"nScope",
};

int   dAttributes = 2;
char *dAttributeName [2] = {
	"qINVALID",
	"qParentScope",
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
                                                  
