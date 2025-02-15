/****************************************************************************
 Copyright (C) 2013 Henry van Merode. All rights reserved.
 Copyright (c) 2015-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef __AX_PU_PARTICLE_3D_POSITION_EMITTER_H__
#define __AX_PU_PARTICLE_3D_POSITION_EMITTER_H__

#include "Particle3D/PU/PUEmitter.h"

NS_AX_BEGIN

class AX_EX_DLL PUPositionEmitter : public PUEmitter
{
public:
    // Constants
    static const bool DEFAULT_RANDOMIZE;

    static PUPositionEmitter* create();
    /**
     */
    bool isRandomized() const;

    /**
     */
    void setRandomized(bool randomized);

    /** Returns a list with positions
     */
    const std::vector<Vec3>& getPositions() const;

    /** Add a new position to this emitter
     */
    void addPosition(const Vec3& position);

    /** Remove all positions from this emitter
     */
    void removeAllPositions();

    /** See ParticleEmitter
     */
    void notifyStart() override;

    /** See ParticleEmitter
     */
    virtual unsigned short calculateRequestedParticles(float timeElapsed) override;

    /** Generate a particle position based on the added positions.
     */
    virtual void initParticlePosition(PUParticle3D* particle) override;

    virtual PUPositionEmitter* clone() override;
    virtual void copyAttributesTo(PUEmitter* emitter) override;

    PUPositionEmitter();
    virtual ~PUPositionEmitter(){};

protected:
    std::vector<Vec3> _positionList;
    bool _randomized;
    size_t _index;
};
NS_AX_END

#endif
