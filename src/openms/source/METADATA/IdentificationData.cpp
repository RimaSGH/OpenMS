// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2017.
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
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------

#include <OpenMS/METADATA/IdentificationData.h>
#include <OpenMS/CHEMISTRY/ProteaseDB.h>

using namespace std;

namespace OpenMS
{
  const Size IdentificationData::MoleculeParentMatch::UNKNOWN_POSITION =
    Size(-1);
  const char IdentificationData::MoleculeParentMatch::UNKNOWN_NEIGHBOR = 'X';
  const char IdentificationData::MoleculeParentMatch::LEFT_TERMINUS = '[';
  const char IdentificationData::MoleculeParentMatch::RIGHT_TERMINUS = ']';


  void IdentificationData::checkScoreTypes_(const ScoreList& scores)
  {
    for (const pair<ScoreTypeRef, double>& score_pair : scores)
    {
      if (!isValidReference_(score_pair.first, score_types_))
      {
        String msg = "invalid reference to a score type - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
  }


  void IdentificationData::checkProcessingSteps_(
    const std::vector<ProcessingStepRef>& step_refs)
  {
    for (ProcessingStepRef step_ref : step_refs)
    {
      if (!isValidReference_(step_ref, processing_steps_))
      {
        String msg = "invalid reference to a data processing step - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
  }


  void IdentificationData::checkParentMatches_(const ParentMatches& matches,
                                               MoleculeType expected_type)
  {
    for (const auto& pair : matches)
    {
      if (!isValidReference_(pair.first, parent_molecules_))
      {
        String msg = "invalid reference to a parent molecule - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
      if (pair.first->molecule_type != expected_type)
      {
        String msg = "unexpected molecule type for parent molecule";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
  }


  IdentificationData::InputFileRef
  IdentificationData::registerInputFile(const String& file)
  {
    return input_files_.insert(file).first;
  }


  IdentificationData::ProcessingSoftwareRef
  IdentificationData::registerDataProcessingSoftware(const Software& software)
  {
    return processing_software_.insert(software).first;
  }


  IdentificationData::SearchParamRef
  IdentificationData::registerDBSearchParam(const DBSearchParam& param)
  {
    // @TODO: any required information that should be checked?
    return db_search_params_.insert(param).first;
  }


  IdentificationData::ProcessingStepRef
  IdentificationData::registerDataProcessingStep(
    const DataProcessingStep& step)
  {
    return registerDataProcessingStep(step, db_search_params_.end());
  }


  IdentificationData::ProcessingStepRef
  IdentificationData::registerDataProcessingStep(
    const DataProcessingStep& step, SearchParamRef search_ref)
  {
    // valid reference to software is required:
    if (!isValidReference_(step.software_ref, processing_software_))
    {
      String msg = "invalid reference to data processing software - register that first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    // if given, references to input files must be valid:
    for (InputFileRef ref : step.input_file_refs)
    {
      if (!isValidReference_(ref, input_files_))
      {
        String msg = "invalid reference to input file - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }

    ProcessingStepRef step_ref = processing_steps_.insert(step).first;
    // if given, reference to DB search param. must be valid:
    if (search_ref != db_search_params_.end())
    {
      if (!isValidReference_(search_ref, db_search_params_))
      {
        String msg = "invalid reference to database search parameters - register those first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
      }
      db_search_steps_.insert(make_pair(step_ref, search_ref));
    }
    return step_ref;
  }


  IdentificationData::ScoreTypeRef
  IdentificationData::registerScoreType(const ScoreType& score)
  {
    pair<ScoreTypes::iterator, bool> result;
    if ((!score.software_opt) && (current_step_ref_ != processing_steps_.end()))
    {
      // transfer the software ref. from the current data processing step:
      const DataProcessingStep& step = *current_step_ref_;
      ScoreType copy(score); // need a copy so we can modify it
      copy.software_opt = step.software_ref;
      result = score_types_.insert(copy);
    }
    else
    {
      // ref. to software may be missing, but must otherwise be valid:
      if (score.software_opt && !isValidReference_(*score.software_opt,
                                                   processing_software_))
      {
        String msg = "invalid reference to data processing software - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
      result = score_types_.insert(score);
    }
    if (!result.second && (score.higher_better != result.first->higher_better))
    {
      String msg = "score type already exists with opposite orientation";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    return result.first;
  }


  IdentificationData::DataQueryRef
  IdentificationData::registerDataQuery(const DataQuery& query)
  {
    // reference to spectrum or feature is required:
    if (query.data_id.empty())
    {
      String msg = "missing identifier in data query";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    // ref. to input file may be missing, but must otherwise be valid:
    if (query.input_file_opt && !isValidReference_(*query.input_file_opt,
                                                   input_files_))
    {
      String msg = "invalid reference to an input file - register that first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    return data_queries_.insert(query).first;
  }


  IdentificationData::IdentifiedPeptideRef
  IdentificationData::registerIdentifiedPeptide(const IdentifiedPeptide&
                                                peptide)
  {
    if (peptide.sequence.empty())
    {
      String msg = "missing sequence for peptide";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    checkParentMatches_(peptide.parent_matches, MoleculeType::PROTEIN);

    return insertIntoMultiIndex_(identified_peptides_, peptide);
  }


  IdentificationData::IdentifiedCompoundRef
  IdentificationData::registerIdentifiedCompound(const IdentifiedCompound&
                                                 compound)
  {
    if (compound.identifier.empty())
    {
      String msg = "missing identifier for compound";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }

    return insertIntoMultiIndex_(identified_compounds_, compound);
  }


  IdentificationData::IdentifiedOligoRef
  IdentificationData::registerIdentifiedOligo(const IdentifiedOligo& oligo)
  {
    if (oligo.sequence.empty())
    {
      String msg = "missing sequence for oligonucleotide";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    checkParentMatches_(oligo.parent_matches, MoleculeType::RNA);

    return insertIntoMultiIndex_(identified_oligos_, oligo);
  }


  IdentificationData::ParentMoleculeRef
  IdentificationData::registerParentMolecule(const ParentMolecule& parent)
  {
    if (parent.accession.empty())
    {
      String msg = "missing accession for parent molecule";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }

    return insertIntoMultiIndex_(parent_molecules_, parent);
  }


  IdentificationData::ParentGroupRef
  IdentificationData::registerParentMoleculeGroup(const ParentMoleculeGroup&
                                                  group)
  {
    for (auto ref : group.parent_molecule_refs)
    {
      if (!isValidReference_(ref, parent_molecules_))
      {
        String msg = "invalid reference to a parent molecule - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }

    return insertIntoMultiIndex_(parent_molecule_groups_, group);
  }


  IdentificationData::QueryMatchRef
  IdentificationData::registerMoleculeQueryMatch(const MoleculeQueryMatch&
                                                 match)
  {
    if (const IdentifiedPeptideRef* ref_ptr =
        boost::get<IdentifiedPeptideRef>(&match.identified_molecule_ref))
    {
      if (!isValidReference_(*ref_ptr, identified_peptides_))
      {
        String msg = "invalid reference to an identified peptide - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
    else if (const IdentifiedCompoundRef* ref_ptr =
             boost::get<IdentifiedCompoundRef>(&match.identified_molecule_ref))
    {
      if (!isValidReference_(*ref_ptr, identified_compounds_))
      {
        String msg = "invalid reference to an identified compound - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
    else if (const IdentifiedOligoRef* ref_ptr =
             boost::get<IdentifiedOligoRef>(&match.identified_molecule_ref))
    {
      if (!isValidReference_(*ref_ptr, identified_oligos_))
      {
        String msg = "invalid reference to an identified oligonucleotide - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }
    if (!isValidReference_(match.data_query_ref, data_queries_))
    {
      String msg = "invalid reference to a data query - register that first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }

    return insertIntoMultiIndex_(query_matches_, match);
  }


  IdentificationData::MatchGroupRef
  IdentificationData::registerQueryMatchGroup(const QueryMatchGroup& group)
  {
    for (auto ref : group.query_match_refs)
    {
      if (!isValidReference_(ref, query_matches_))
      {
        String msg = "invalid reference to a molecule-query match - register that first";
        throw Exception::IllegalArgument(__FILE__, __LINE__,
                                         OPENMS_PRETTY_FUNCTION, msg);
      }
    }

    return insertIntoMultiIndex_(query_match_groups_, group);
  }


  void IdentificationData::addScore(QueryMatchRef match_ref,
                                    ScoreTypeRef score_ref, double value)
  {
    if (!isValidReference_(score_ref, score_types_))
    {
      String msg = "invalid reference to a score type - register that first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }

    ModifyMultiIndexAddScore<MoleculeQueryMatch> modifier(score_ref, value);
    query_matches_.modify(match_ref, modifier);
  }


  void IdentificationData::setCurrentProcessingStep(ProcessingStepRef step_ref)
  {
    if (!isValidReference_(step_ref, processing_steps_))
    {
      String msg = "invalid reference to a processing step - register that first";
      throw Exception::IllegalArgument(__FILE__, __LINE__,
                                       OPENMS_PRETTY_FUNCTION, msg);
    }
    current_step_ref_ = step_ref;
  }


  IdentificationData::ProcessingStepRef
  IdentificationData::getCurrentProcessingStep()
  {
    return current_step_ref_;
  }


  void IdentificationData::clearCurrentProcessingStep()
  {
    current_step_ref_ = processing_steps_.end();
  }


  IdentificationData::ScoreTypeRef IdentificationData::findScoreType(
    const String& score_name) const
  {
    return findScoreType(score_name, processing_software_.end());
  }


  IdentificationData::ScoreTypeRef IdentificationData::findScoreType(
    const String& score_name, ProcessingSoftwareRef software_ref) const
  {
    for (ScoreTypeRef it = score_types_.begin(); it != score_types_.end(); ++it)
    {
      if ((it->name == score_name) &&
          ((software_ref == processing_software_.end()) ||
           (it->software_opt == software_ref)))
      {
        return it;
      }
    }
    return score_types_.end();
  }


  vector<IdentificationData::QueryMatchRef>
  IdentificationData::getBestMatchPerQuery(ScoreTypeRef score_ref) const
  {
    vector<QueryMatchRef> results;
    bool higher_better = score_ref->higher_better;
    pair<double, bool> best_score = make_pair(0.0, false);
    QueryMatchRef best_ref = query_matches_.end();
    for (QueryMatchRef ref = query_matches_.begin();
         ref != query_matches_.end(); ++ref)
    {
      pair<double, bool> current_score = ref->getScore(score_ref);
      if ((best_ref != query_matches_.end()) &&
          (ref->data_query_ref != best_ref->data_query_ref))
      {
        // finalize previous query:
        if (best_score.second) results.push_back(best_ref);
        best_score = current_score;
        best_ref = ref;
      }
      else if (current_score.second &&
               (!best_score.second ||
                isBetterScore(current_score.first, best_score.first,
                              higher_better)))
      {
        // new best score for the current query:
        best_score = current_score;
        best_ref = ref;
      }
    }
    // finalize last query:
    if (best_score.second) results.push_back(best_ref);

    return results;
  }


  void IdentificationData::calculateCoverages(bool check_molecule_length)
  {
    // aggregate molecule-parent matches by parent:
    struct ParentData
    {
      Size length;
      double coverage;
      vector<pair<Size, Size>> fragments;
    };
    map<ParentMoleculeRef, ParentData> parent_info;

    // go through all peptides:
    for (const auto& molecule : identified_peptides_)
    {
      Size molecule_length = check_molecule_length ?
        molecule.sequence.size() : 0;
      for (const auto& pair : molecule.parent_matches)
      {
        auto pos = parent_info.find(pair.first);
        if (pos == parent_info.end()) // new parent molecule
        {
          ParentData pd;
          pd.length = AASequence::fromString(pair.first->sequence).size();
          if (pd.length == 0) break; // sequence not available
          pos = parent_info.insert(make_pair(pair.first, pd)).first;
        }
        Size parent_length = pos->second.length; // always check this
        for (const auto& match : pair.second)
        {
          if (match.hasValidPositions(molecule_length, parent_length))
          {
            pos->second.fragments.push_back(make_pair(match.start_pos,
                                                      match.end_pos));
          }
        }
      }
    }
    // go through all oligonucleotides:
    for (const auto& molecule : identified_oligos_)
    {
      Size molecule_length = check_molecule_length ?
        molecule.sequence.size() : 0;
      for (const auto& pair : molecule.parent_matches)
      {
        auto pos = parent_info.find(pair.first);
        if (pos == parent_info.end()) // new parent molecule
        {
          ParentData pd;
          pd.length = NASequence::fromString(pair.first->sequence).size();
          if (pd.length == 0) break; // sequence not available
          pos = parent_info.insert(make_pair(pair.first, pd)).first;
        }
        Size parent_length = pos->second.length; // always check this
        for (const auto& match : pair.second)
        {
          if (match.hasValidPositions(molecule_length, parent_length))
          {
            pos->second.fragments.push_back(make_pair(match.start_pos,
                                                      match.end_pos));
          }
        }
      }
    }

    // calculate coverage for each parent:
    for (auto& pair : parent_info)
    {
      vector<bool> covered(pair.second.length, false);
      for (const auto& fragment : pair.second.fragments)
      {
        fill(covered.begin() + fragment.first,
             covered.begin() + fragment.second + 1, true);
      }
      pair.second.coverage = (accumulate(covered.begin(), covered.end(), 0) /
                              double(pair.second.length));
    }
    // set coverage:
    for (ParentMoleculeRef ref = parent_molecules_.begin();
         ref != parent_molecules_.end(); ++ref)
    {
      auto pos = parent_info.find(ref);
      double coverage = (pos == parent_info.end()) ? 0.0 : pos->second.coverage;
      parent_molecules_.modify(ref, [coverage](ParentMolecule& parent)
                               {
                                 parent.coverage = coverage;
                               });
    }
  }


  void IdentificationData::cleanup(bool require_query_match,
                                   bool require_identified_sequence,
                                   bool require_parent_match,
                                   bool require_parent_group,
                                   bool require_match_group)
  {
    // we expect that only "primary results" (stored in classes derived from
    // "ScoredProcessingResult") will be directly removed (by filters) - not
    // meta data (incl. data queries, score types, processing steps etc.)

    // remove parent molecules based on parent groups:
    if (require_parent_group)
    {
      set<ParentMoleculeRef> parent_refs;
      for (const auto& group : parent_molecule_groups_)
      {
        parent_refs.insert(group.parent_molecule_refs.begin(),
                           group.parent_molecule_refs.end());
      }
      removeFromSetIfMissing_(parent_molecules_, parent_refs);
    }

    // remove parent matches based on parent molecules:
    ModifyMultiIndexRemoveParentMatches<IdentifiedPeptide>
      pep_modifier(parent_molecules_);
    for (auto it = identified_peptides_.begin();
         it != identified_peptides_.end(); ++it)
    {
      identified_peptides_.modify(it, pep_modifier);
    }
    ModifyMultiIndexRemoveParentMatches<IdentifiedOligo>
      oli_modifier(parent_molecules_);
    for (auto it = identified_oligos_.begin();
         it != identified_oligos_.end(); ++it)
    {
      identified_oligos_.modify(it, oli_modifier);
    }

    // remove identified molecules based on parent matches:
    if (require_parent_match)
    {
      removeFromSetIf_(identified_peptides_, [](IdentifiedPeptides::iterator it)
                       {
                         return it->parent_matches.empty();
                       });
      removeFromSetIf_(identified_oligos_, [](IdentifiedOligos::iterator it)
                       {
                         return it->parent_matches.empty();
                       });
    }

    // remove molecule-query matches based on identified molecules:
    set<IdentifiedMoleculeRef> id_refs;
    for (auto it = identified_peptides_.begin();
         it != identified_peptides_.end(); ++it)
    {
      id_refs.insert(it);
    }
    for (auto it = identified_compounds_.begin();
         it != identified_compounds_.end(); ++it)
    {
      id_refs.insert(it);
    }
    for (auto it = identified_oligos_.begin();
         it != identified_oligos_.end(); ++it)
    {
      id_refs.insert(it);
    }
    removeFromSetIf_(query_matches_, [&](MoleculeQueryMatches::iterator it)
                     {
                       return !id_refs.count(it->identified_molecule_ref);
                     });

    // remove molecule-query matches based on query match groups:
    if (require_match_group)
    {
      set<QueryMatchRef> match_refs;
      for (const auto& group : query_match_groups_)
      {
        match_refs.insert(group.query_match_refs.begin(),
                          group.query_match_refs.end());
      }
      removeFromSetIfMissing_(query_matches_, match_refs);
    }

    // remove id'd molecules and data queries based on molecule-query matches:
    if (require_query_match)
    {
      set<DataQueryRef> query_refs;
      set<IdentifiedPeptideRef> peptide_refs;
      set<IdentifiedCompoundRef> compound_refs;
      set<IdentifiedOligoRef> oligo_refs;
      for (const auto& match : query_matches_)
      {
        query_refs.insert(match.data_query_ref);
        IdentificationData::MoleculeType molecule_type =
          match.getMoleculeType();
        if (molecule_type == IdentificationData::MoleculeType::PROTEIN)
        {
          peptide_refs.insert(match.getIdentifiedPeptideRef());
        }
        else if (molecule_type == IdentificationData::MoleculeType::COMPOUND)
        {
          compound_refs.insert(match.getIdentifiedCompoundRef());
        }
        else if (molecule_type == IdentificationData::MoleculeType::RNA)
        {
          oligo_refs.insert(match.getIdentifiedOligoRef());
        }
      }
      removeFromSetIfMissing_(data_queries_, query_refs);
      removeFromSetIfMissing_(identified_peptides_, peptide_refs);
      removeFromSetIfMissing_(identified_compounds_, compound_refs);
      removeFromSetIfMissing_(identified_oligos_, oligo_refs);
    }

    // remove parent molecules based on identified molecules:
    if (require_identified_sequence)
    {
      set<IdentificationData::ParentMoleculeRef> parent_refs;
      for (const auto& peptide : identified_peptides_)
      {
        for (const auto& parent_pair : peptide.parent_matches)
        {
          parent_refs.insert(parent_pair.first);
        }
      }
      for (const auto& oligo : identified_oligos_)
      {
        for (const auto& parent_pair : oligo.parent_matches)
        {
          parent_refs.insert(parent_pair.first);
        }
      }
      removeFromSetIfMissing_(parent_molecules_, parent_refs);
    }

    // remove entries from parent molecule groups based on parent molecules:
    bool warn = false;
    for (auto group_it = parent_molecule_groups_.begin();
         group_it != parent_molecule_groups_.end(); )
    {
      Size old_size = group_it->parent_molecule_refs.size();
      // nested lambda expressions, oh my!
      parent_molecule_groups_.modify(
        group_it, [&](ParentMoleculeGroup& group)
        {
          removeFromSetIf_(group.parent_molecule_refs,
                           [&](set<ParentMoleculeRef>::iterator it) -> bool
                           {
                             return !isValidReference_(*it, parent_molecules_);
                           });
        });
      if (group_it->parent_molecule_refs.empty())
      {
        group_it = parent_molecule_groups_.erase(group_it);
      }
      else
      {
        if (group_it->parent_molecule_refs.size() != old_size)
        {
          warn = true;
        }
        ++group_it;
      }
    }
    if (warn)
    {
      LOG_WARN << "Warning: filtering removed elements from parent molecule groups - associated scores may not be valid any more" << endl;
    }

    // remove entries from query match groups based on molecule-query matches:
    warn = false;
    for (auto group_it = query_match_groups_.begin();
         group_it != query_match_groups_.end(); )
    {
      Size old_size = group_it->query_match_refs.size();
      // and more nested lambda expressions!
      query_match_groups_.modify(
        group_it, [&](QueryMatchGroup& group)
        {
          removeFromSetIf_(group.query_match_refs,
                           [&](set<QueryMatchRef>::iterator it) -> bool
                           {
                             return !isValidReference_(*it, query_matches_);
                           });
        });
      if (group_it->query_match_refs.empty())
      {
        group_it = query_match_groups_.erase(group_it);
      }
      else
      {
        if (group_it->query_match_refs.size() != old_size)
        {
          warn = true;
        }
        ++group_it;
      }
    }
    if (warn)
    {
      LOG_WARN << "Warning: filtering removed elements from query match groups - associated scores may not be valid any more" << endl;
    }
  }

} // end namespace OpenMS
