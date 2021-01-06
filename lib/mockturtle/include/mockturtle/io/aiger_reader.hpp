/* mockturtle: C++ logic network library
 * Copyright (C) 2018  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file aiger_reader.hpp
  \brief Lorina reader for AIGER files

  \author Mathias Soeken
*/

#pragma once

#include <lorina/aiger.hpp>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Lorina reader callback for Aiger files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `get_constant`
 * - `create_not`
 * - `create_and`
 *
   \verbatim embed:rst
  
   Example
   
   .. code-block:: c++
   
      aig_network aig;
      lorina::read_aiger( "file.aig", aiger_reader( aig ) );

      mig_network mig;
      lorina::read_aiger( "file.aig", aiger_reader( mig ) );
   \endverbatim
 */
template<typename Ntk>
class aiger_reader : public lorina::aiger_reader
{
public:
  explicit aiger_reader( Ntk& ntk ) : _ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_ro_v<Ntk>, "Ntk does not implement the create_ro function" );
    static_assert( has_create_ri_v<Ntk>, "Ntk does not implement the create_ri function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and function" );
  }

  ~aiger_reader()
  {
    for ( auto lit : outputs )
    {
      auto signal = signals[lit >> 1];
      if ( lit & 1 )
      {
        signal = _ntk.create_not( signal );
      }
      _ntk.create_po( signal );
    }
      auto count = 0;
      for ( auto latch : latches )
      {
          auto const lit = std::get<0>( latch );
          auto const reset = std::get<1>( latch );

          auto signal = signals[lit >> 1];
          if ( lit & 1 )
          {
              signal = _ntk.create_not( signal );
          }
          _ntk.create_ri( signal, reset );
      }
  }

//  void on_input_name (unsigned index, const std::string name) const override
//  {
//    (void)index;
//    (void)name;
//
//    _ntk.create_input_name()
//  }
//
//  void on_output_name (unsigned index, const std::string name) const  override{
//
//  }

  void on_header( std::size_t, std::size_t num_inputs, std::size_t num_latches, std::size_t, std::size_t ) const override
  {
    //assert( num_latches == 0 && "AIG has latches, not supported yet." );

    /* constant */
    signals.push_back( _ntk.get_constant( false ) );

    /* create inputs */
    for ( auto i = 0u; i < num_inputs; ++i )
    {
      signals.push_back( _ntk.create_pi() );
    }

      /* create latch outputs (ro) */
      for ( auto i = 0u; i < num_latches; ++i )
      {
          signals.push_back( _ntk.create_ro() );
      }
  }

  void on_and( unsigned index, unsigned left_lit, unsigned right_lit ) const override
  {
    (void)index;
    assert( signals.size() == index );

    auto left = signals[left_lit >> 1];
    if ( left_lit & 1 )
    {
      left = _ntk.create_not( left );
    }

    auto right = signals[right_lit >> 1];
    if ( right_lit & 1 )
    {
      right = _ntk.create_not( right );
    }

    signals.push_back( _ntk.create_and( left, right ) );
  }

    void on_latch( unsigned index, unsigned next, latch_init_value reset ) const override
    {
        (void)index;
        int8_t r = reset == latch_init_value::NONDETERMINISTIC ? -1 : (reset == latch_init_value::ONE ? 1 : 0);
        latches.push_back( std::make_tuple( next, r, "" ) );
    }

  void on_output( unsigned index, unsigned lit ) const override
  {
    assert( index == outputs.size() );
    outputs.push_back( lit );
  }

  /*virtual void on_input_name( unsigned index, const std::string& name ) const
  {
    (void)index;
    (void)name;
    _ntk.create_in_name(index, name);
    inputNames[index] = name;
  }

  virtual void on_output_name( unsigned index, const std::string& name ) const
  {
    (void)index;
    (void)name;
    _ntk.create_out_name(index, name);
    outputNames[index] = name;
  }*/

private:
  Ntk& _ntk;

  mutable std::vector<unsigned> outputs;
  mutable std::map<int, std::string> inputNames;
  mutable std::map<int, std::string> outputNames;
  mutable std::vector<signal<Ntk>> signals;
  mutable std::vector<std::tuple<unsigned,int8_t,std::string>> latches;
};

} /* namespace mockturtle */
