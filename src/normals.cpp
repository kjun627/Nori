#include <nori/integrator.h>

NORI_NAMESPACE_BEGIN

class NormalIntegrator : public Integrator {
public:
    NormalIntegrator(const PropertyList &props){
        m_myProperty = props.getString("myProperty");
        std::cout << "parameter value was : " << m_myProperty << std::endl;
    }

    Color3f  Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const{
        return Color3f(0,1,0);
    }

    std::string toString() const{
        return tfm::format(
            "NoramlIntegrator[\n"
        " myProperty = \"%s\"\n"
        "]",
        m_myProperty
        );
    }
protected:
    std::string m_myProperty;
};

NORI_REGISTER_CLASS(NormalIntegrator, "normals");
NORI_NAMESPACE_END
