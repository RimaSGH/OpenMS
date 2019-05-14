// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2018.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Juliane Schmachtenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/QC/QCBase.h>

namespace OpenMS
{
  class FeatureMap;
  class TransformationDescription;

  /**
    @brief Take the original retention time before map alignment and use the transformation information of the alignment's trafoXML
           for calculation of the post map alignment retention times.
           
    Sets meta values "rt_raw" and "rt_align" in PeptideIdentifications of the featureMap.
    
    @param trafo: Transformation information of map alignment
    @param features: featureMap before map alignment, contains original retention time
    **/
  class OPENMS_DLLAPI RTAlignment : public QCBase
  {
    public:
    /// Constructor
    RTAlignment() = default;
    
    /// Destructor
    virtual ~RTAlignment() = default;

    /**
     @brief Calculates post map alignment retention time
     and sets meta values "rt_raw" and "rt_align" in PeptideIdentification
     @param features: FeatureMap where the meta values are annotated
     @param trafo: Transformation information to get needed data from
    **/
    void compute(FeatureMap& features, const TransformationDescription& trafo);
    
    /// returns the name of the metric
    const String& getName() const override;
    
    /// define the required input file: featureXML before map alignment (=POSTFDRFEAT), trafoXML after map alignment (=TRAFOALIGN)
    Status requires() const override;
    
  private:
    /// name of the metric
    const String name_ = "RTAlignment";
  };
}
