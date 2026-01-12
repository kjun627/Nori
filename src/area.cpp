/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/emitter.h>
#include <nori/mesh.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Area light emitter
 * 
 * This emitter turns any mesh into an area light source that
 * uniformly emits radiance from its surface.
 */
class AreaEmitter : public Emitter {
public:
    AreaEmitter(const PropertyList &props) {
        m_radiance = Color3f(0.0f);
        m_radiance = props.getColor("radiance", Color3f(0.0f)); 
    }

    virtual std::string toString() const override {
        return tfm::format(
            "AreaEmitter[\n"
            "  radiance = %s\n"
            "]",
            m_radiance.toString()
        );
    }

protected:
    Color3f m_radiance;  ///< Radiance emitted by the area light
};

NORI_REGISTER_CLASS(AreaEmitter, "area");
NORI_NAMESPACE_END
