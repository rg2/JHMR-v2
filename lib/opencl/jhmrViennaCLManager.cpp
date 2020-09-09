
#include "jhmrViennaCLManager.h"

#include <tuple>

#include <boost/container/flat_set.hpp>

#include <viennacl/ocl/backend.hpp>

void jhmr::setup_vcl_ctx_if_needed(const long vcl_ctx,
                                   const boost::compute::context& ctx,
                                   const boost::compute::command_queue& queue)
{
  using Key = std::tuple<long,cl_context,cl_command_queue>;

  static boost::container::flat_set<Key> init_set;
  
  Key k(vcl_ctx,ctx.get(),queue.get());

  if (init_set.find(k) == init_set.end())
  {
    viennacl::ocl::setup_context(vcl_ctx, ctx, ctx.get_device().id(), queue);
    init_set.insert(k);
  }
}

