
#ifndef __SOP_Analysis_h__
#define __SOP_Analysis_h__

#include <SOP/SOP_Node.h>

class SOP_TemplateA : public SOP_Node
{
public:
	     SOP_TemplateA(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_TemplateA();

    static PRM_Template		 myTemplateList[];
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);

protected:
    virtual const char          *inputLabel(unsigned idx) const;

    /// Method to cook geometry for the SOP
    virtual OP_ERROR		 cookMySop(OP_Context &context);

private:
	void buildPolygon();
	void buildTube();
	void buildMetaBall();
	void buildNurbsCurve();
	GU_Detail* packedGdp;
	GU_DetailHandle* packedGdpHandle;
};

#endif
