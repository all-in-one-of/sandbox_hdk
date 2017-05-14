
#ifndef __SOP_Analysis_h__
#define __SOP_Analysis_h__

#include <SOP/SOP_Node.h>

class SOP_Laplacian_Smooth : public SOP_Node
{
public:
	     SOP_Laplacian_Smooth(OP_Network *net, const char *name, OP_Operator *op);
    virtual ~SOP_Laplacian_Smooth();

    static PRM_Template		 myTemplateList[];
    static OP_Node		*myConstructor(OP_Network*, const char *,
							    OP_Operator *);

protected:
    virtual const char          *inputLabel(unsigned idx) const;

    /// Method to cook geometry for the SOP
    virtual OP_ERROR		 cookMySop(OP_Context &context);

private:
    void	ATTRIB_NAME(UT_String &str){ evalString(str, "attribute_name", 0, 0); }
    fpreal	FREQUENCY(fpreal t)		{ return evalFloat("frequency", 0, t); }
    int		INTERATIONS(fpreal t)		{ return evalInt("iterations", 0, t); }

};

#endif
