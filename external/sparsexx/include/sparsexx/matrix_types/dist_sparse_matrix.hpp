#pragma once
#include <sparsexx/matrix_types/type_traits.hpp>
#include <sparsexx/util/mpi.hpp>
#include <sparsexx/util/submatrix.hpp>

#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>
#include <memory>

namespace sparsexx {



template <typename SpMatType>
class dist_sparse_matrix {

public:

  using value_type = detail::value_type_t<SpMatType>;
  using index_type = detail::index_type_t<SpMatType>;
  using size_type  = detail::size_type_t<SpMatType>;
  using tile_type  = SpMatType;

protected:

  MPI_Comm comm_;
  int comm_size_;
  int comm_rank_;

  size_type global_m_;
  size_type global_n_;

  std::shared_ptr<tile_type> diagonal_tile_     = nullptr;
  std::shared_ptr<tile_type> off_diagonal_tile_ = nullptr;

  std::vector< std::pair<index_type,index_type> > dist_row_extents_;

public:

  constexpr dist_sparse_matrix() noexcept = default;
  dist_sparse_matrix( dist_sparse_matrix&& ) noexcept = default;

  dist_sparse_matrix( MPI_Comm c, size_type M, size_type N ) :
    comm_(c), global_m_(M), global_n_(N) {

    comm_size_ = detail::get_mpi_size(comm_);
    comm_rank_ = detail::get_mpi_rank(comm_);
    const auto nrow_per_rank = M / comm_size_;

    dist_row_extents_.resize(comm_size_);
    dist_row_extents_[0] = { 0, nrow_per_rank };
    for( int i = 1; i < comm_size_; ++i ) {
      dist_row_extents_[i] = {
        dist_row_extents_[i-1].second,
        dist_row_extents_[i-1].second + nrow_per_rank
      };
    }
    dist_row_extents_.back().second += M % comm_size_; // Last rank gets carry-over
  
  }

  dist_sparse_matrix( const dist_sparse_matrix& other ) :
    dist_sparse_matrix( other.comm_, other.global_m_, other.global_n_ ) {

    if( other.diagonal_tile_ ) set_diagonal_tile( other.diagonal_tile() );
    if( other.off_diagonal_tile_ ) 
      set_off_diagonal_tile( other.off_diagonal_tile() );

  }

  dist_sparse_matrix( MPI_Comm c, const SpMatType& A ) :
    dist_sparse_matrix( c, A.m(), A.n() ) {

    auto [local_row_st, local_row_en] = dist_row_extents_[comm_rank_];
    auto local_lo = std::make_pair<int64_t,int64_t>(local_row_st, local_row_st);
    auto local_up = std::make_pair<int64_t,int64_t>(local_row_en, local_row_en);
    diagonal_tile_ = std::make_shared<tile_type>(
      extract_submatrix( A, local_lo, local_up )
    );
    off_diagonal_tile_ = std::make_shared<tile_type>(
      extract_submatrix_inclrow_exclcol( A, local_lo, local_up )
    );
    diagonal_tile_->set_indexing(0);
    off_diagonal_tile_->set_indexing(0);
      

  }

  inline auto m() const { return global_m_; }
  inline auto n() const { return global_n_; }

  inline MPI_Comm comm() const { return comm_; }

  inline auto row_bounds(int rank) const {return dist_row_extents_[rank]; }

  inline size_type row_extent(int rank) const {
    return dist_row_extents_[rank].second -
           dist_row_extents_[rank].first; 
  }

  inline size_type local_row_extent() const { 
    return row_extent(comm_rank_);
  }

  inline size_type local_row_start() const {
    return dist_row_extents_[comm_rank_].first;
  }

  inline size_type nnz() const noexcept {
    size_t _nnz = 0;
    if( diagonal_tile_ ) _nnz += diagonal_tile_->nnz();
    if( off_diagonal_tile_ ) _nnz += off_diagonal_tile_->nnz();
    return _nnz;
  }

  inline size_type mem_footprint() const noexcept {
    size_type _mf = 0;
    if( diagonal_tile_ ) _mf += diagonal_tile_->mem_footprint();
    if( off_diagonal_tile_ ) _mf += off_diagonal_tile_->mem_footprint();
    return _mf;
  }

  auto       diagonal_tile_ptr()       { return diagonal_tile_; }
  const auto diagonal_tile_ptr() const { return diagonal_tile_; }
  auto       off_diagonal_tile_ptr()       { return off_diagonal_tile_; }
  const auto off_diagonal_tile_ptr() const { return off_diagonal_tile_; }

  const auto& diagonal_tile() const { return *diagonal_tile_; }
  const auto& off_diagonal_tile() const { return *off_diagonal_tile_; }

  void set_diagonal_tile( const SpMatType& A ) {
    diagonal_tile_ = std::make_shared<tile_type>( A );
  }
  void set_off_diagonal_tile( const SpMatType& A ) {
    off_diagonal_tile_ = std::make_shared<tile_type>( A );
  }

  void set_diagonal_tile( SpMatType&& A ) {
    diagonal_tile_ = std::make_shared<tile_type>( std::move(A) );
  }
  void set_off_diagonal_tile( SpMatType&& A ) {
    off_diagonal_tile_ = std::make_shared<tile_type>( std::move(A) );
  }
}; // class dist_sparse_matrix




}
