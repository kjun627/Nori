#include <nori/integrator.h>
#include <nori/scene.h>

NORI_NAMESPACE_BEGIN
class SimpleIntegrator : public Integrator{
public:
    SimpleIntegrator(const PropertyList& props){  
        m_position = props.getPoint("position");
        m_energy = props.getColor("energy");
    }
    Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray)const{
        Intersection its;
        if(!scene->rayIntersect(ray,its)) return Color3f(0.0f);
                
        Point3f dist = m_position - its.p;
        float distSquare = dist.squaredNorm();
        Vector3f lightDir = dist.normalized();
        
        Ray3f shadowRay(its.p,lightDir);
        shadowRay.mint = Epsilon;
        shadowRay.maxt = dist.norm();
        Intersection itsForShadow;
        if(scene->rayIntersect(shadowRay,its)){
            return Color3f(0.0f);
        }
        

        float cosTheta = its.shFrame.n.dot(lightDir);

        float reflecMulDist = 1/(distSquare * 4 * pow(M_PI,2));
        Color3f lightGoToCamera = m_energy * std::max(0.0f,cosTheta) * reflecMulDist;
        
        return lightGoToCamera;
    }
    std::string toString() const{
        return "SimpleIntegrator[]";
    }
private:
    Point3f m_position;
    Color3f m_energy;
};
NORI_REGISTER_CLASS(SimpleIntegrator, "simple");
NORI_NAMESPACE_END