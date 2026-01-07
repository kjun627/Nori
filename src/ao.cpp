#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN
class AoIntegrator : public Integrator{
public:
    AoIntegrator(const PropertyList& props){  
    }
    Color3f Li(const Scene* scene, Sampler* sampler, const Ray3f& ray)const{
        Intersection its;
        if(!scene->rayIntersect(ray,its)) return Color3f(0.0f);
                
        Frame standard(its.shFrame.n);
        
        Point2f sample = sampler->next2D();
        Vector3f localDir = Warp::squareToCosineHemisphere(sample);
        Vector3f WorldDir = standard.toWorld(localDir);

        Ray3f aoRay(its.p, WorldDir);
        Intersection itsForShadow;
        if(scene->rayIntersect(aoRay, itsForShadow)){
            return Color3f(0.0f);
        } 

        return Color3f(1.0f);
    }
    std::string toString() const{
        return "AoIntegrator[]";
    }
};
NORI_REGISTER_CLASS(AoIntegrator, "ao");
NORI_NAMESPACE_END