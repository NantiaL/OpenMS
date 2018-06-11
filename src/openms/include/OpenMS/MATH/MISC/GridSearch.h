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
// $Maintainer: Julianus Pfeuffer $
// $Authors: Julianus Pfeuffer $
// --------------------------------------------------------------------------

#ifndef OPENMS_MATH_MISC_GRIDSEARCH_H
#define OPENMS_MATH_MISC_GRIDSEARCH_H

#include <array>
#include <iostream>
#include <vector>
#include <cmath>
#include <tuple>

// The general class template
template <size_t param_index, size_t grid_size, typename EvalResult, typename Tuple, typename... TupleTypes>
struct Looper
{
};

// Specialization for the base case
// - shape_index == shape_size
// - TupleTypes is empty here
// - All indices in Functor are bound (i.e. can be called with empty arguments)
template <size_t grid_size, typename EvalResult, typename Tuple, typename... TupleTypes>
struct Looper<grid_size, grid_size, EvalResult, Tuple, TupleTypes...>
{
  template <typename Functor>
  double operator()(const Tuple&, Functor functor, EvalResult /*bestValue*/, std::array<size_t, grid_size>& /*bestIndices*/)
  {
    return functor();
  }
};

// Specialization for the loop case
// - increment shape_index
// - create new Functor with one argument less and the first being bound to it
// - loop over all values in the current vector and update best score and best indices
template <size_t param_index, size_t grid_size, typename EvalResult, typename Tuple, typename FirstTupleType, typename... TupleTypes>
struct Looper<param_index, grid_size, EvalResult, Tuple, FirstTupleType, TupleTypes...>
{
  template <typename Functor>
  EvalResult operator()(const Tuple& grid, Functor functor, EvalResult bestValue, std::array<size_t, grid_size>& bestIndices)
  {
    for (size_t index = 0; index < std::get<param_index>(grid).size(); ++index)
    {
      double currVal = Looper<param_index + 1, grid_size, EvalResult, Tuple, TupleTypes...>()
          (
              grid,
              [&grid, index, &functor](TupleTypes... rest){ return functor(std::get<param_index>(grid)[index], rest...);},
              bestValue,
              bestIndices
          );

      if ( currVal > bestValue )
      {
        bestValue = currVal;
        bestIndices[param_index] = index;
      }
    }
    return bestValue;
  }
};



template <typename... TupleTypes>
class GridSearch
{
public:
  explicit GridSearch(std::vector<TupleTypes>... gridValues):
      grid_(std::make_tuple<std::vector<TupleTypes>...>(std::move(gridValues)...))
  {}

  //Specific implementation for function objects
  template <typename Functor>
  typename std::result_of<Functor(TupleTypes...)>::type evaluate(Functor evaluator,
                                                                 typename std::result_of<Functor(TupleTypes...)>::type startValue,
                                                                 std::array<size_t,std::tuple_size<std::tuple<std::vector<TupleTypes>...>>::value>& resultIndices)
  {
    return Looper<0,
        std::tuple_size<std::tuple<std::vector<TupleTypes>...>>::value,
        typename std::result_of<Functor(TupleTypes...)>::type,
        std::tuple<std::vector<TupleTypes>...>,
        TupleTypes...> ()
        (grid_, evaluator, startValue, resultIndices);
  }


  //Specific implementation for function pointers
  template <typename EvalResult>
  EvalResult evaluate(EvalResult evaluator(TupleTypes...),
                      EvalResult startValue,
                      std::array<size_t,std::tuple_size<std::tuple<std::vector<TupleTypes>...>>::value>& resultIndices)
  {
    return Looper<0,
        std::tuple_size<std::tuple<std::vector<TupleTypes>...>>::value,
        EvalResult,
        std::tuple<std::vector<TupleTypes>...>,
        TupleTypes...>()
        (grid_, evaluator, startValue, resultIndices);
  }

private:
  std::tuple<std::vector<TupleTypes>...> grid_;
};


/* Example
  double evaluator(double i, std::string j, double k, double l)
  {
      return i+j.length()+k+l;
  }

  // Program
  double wrongevaluator(std::string i, std::string j, double k, double l)
  {
      return i.length()+j.length()+k+l;
  }


  struct Evaluator
  {
   double operator()(double i, std::string j, double k, double l){return i+j.length()+k+l;};
  };

  int main() {
      GridSearch<double, std::string, double, double> gs({1,3,5,2},{"foo","barz"},{2},{3});
      std::array<size_t, 4> bestParamIdx{{0u,0u,0u,0u}};
      std::cout << "Best value " <<  gs.evaluate(wrongevaluator, -1.0, bestParamIdx) << " at: ";
      for (auto idx : bestParamIdx)
      {
          std::cout << idx << ", ";
      }
  }
}*/

#endif //OPENMS_MATH_MISC_GRIDSEARCH_H
