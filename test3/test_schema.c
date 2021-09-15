
/* Generated automatically by schema */

/* Private Data */

static short dAttributeOffset [2][1] = {
	 -1, 
	 -1, 
};

static short dAttributeType [2][1] = {
	 -1, 
	 -1, 
};

static short dAttributeTags [2][1] = {
	  0, 
	  0, 
};

static short dClassIsA [2][2] = {
	1, 0, 
	0, 1, 
};


/* Public Data */

short dObjectSize [2] = {
	0, 0, 
};

int   dObjects = 2;
char *dObjectName [2] = {
	"nINVALID",
	"Object",
};

int   dAttributes = 1;
char *dAttributeName [1] = {
	"qINVALID",
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
                                                  
