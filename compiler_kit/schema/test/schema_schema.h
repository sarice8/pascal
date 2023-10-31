/* Generated automatically by schema */

#define qINVALID(N)		(Integer4)	GetAttr(T_qINVALID, N)
#define qClassTree(N)		(List)	GetAttr(T_qClassTree, N)
#define qAttrSyms(N)		(List)	GetAttr(T_qAttrSyms, N)
#define qIdent(N)		(Integer4)	GetAttr(T_qIdent, N)
#define qText(N)		(StringN)	GetAttr(T_qText, N)
#define qCode(N)		(Integer4)	GetAttr(T_qCode, N)
#define qAttrs(N)		(List)	GetAttr(T_qAttrs, N)
#define qDerived(N)		(List)	GetAttr(T_qDerived, N)
#define qAttrSym(N)		(Node)	GetAttr(T_qAttrSym, N)
#define qConstraint(N)		(Node)	GetAttr(T_qConstraint, N)
#define qTags(N)		(Integer4)	GetAttr(T_qTags, N)
#define qType(N)		(Integer4)	GetAttr(T_qType, N)

#define SetqINVALID(N,V)		SetAttr(T_qINVALID, N, (void*)(V))
#define SetqClassTree(N,V)		SetAttr(T_qClassTree, N, (void*)(V))
#define SetqAttrSyms(N,V)		SetAttr(T_qAttrSyms, N, (void*)(V))
#define SetqIdent(N,V)		SetAttr(T_qIdent, N, (void*)(V))
#define SetqText(N,V)		SetAttr(T_qText, N, (void*)(V))
#define SetqCode(N,V)		SetAttr(T_qCode, N, (void*)(V))
#define SetqAttrs(N,V)		SetAttr(T_qAttrs, N, (void*)(V))
#define SetqDerived(N,V)		SetAttr(T_qDerived, N, (void*)(V))
#define SetqAttrSym(N,V)		SetAttr(T_qAttrSym, N, (void*)(V))
#define SetqConstraint(N,V)		SetAttr(T_qConstraint, N, (void*)(V))
#define SetqTags(N,V)		SetAttr(T_qTags, N, (void*)(V))
#define SetqType(N,V)		SetAttr(T_qType, N, (void*)(V))

#define NewnINVALID()		NewNode(nINVALID)
#define NewObject()		NewNode(Object)
#define NewnSchema()		NewNode(nSchema)
#define NewnClass()		NewNode(nClass)
#define NewnAttr()		NewNode(nAttr)
#define NewnConstraint()		NewNode(nConstraint)
#define NewnAttrSym()		NewNode(nAttrSym)

