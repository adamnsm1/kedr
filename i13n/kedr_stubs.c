/*
 * The thunk functions for the events. The purpose of these functions is
 * to prepare the information about the event, store it in an instance of
 * struct kedr_local and pass to the handler stub.
 *
 * The stubs do nothing by themselves but they will contain the Ftrace
 * placeholders if CONFIG_FUNCTION_TRACER is set.
 * This will allow replacing them with the real handlers in runtime,
 * similar to how Livepatch does its job.
 *
 * Compile this file and link to each binary you instrument with KEDR.
 * Do not forget to add the path to the KEDR header files to the compiler
 * options.
 */

#include <linux/compiler.h>	/* notrace, ... */
#include <linux/stddef.h>	/* NULL */
#include <linux/slab.h>		/* k*alloc(), k*free(), ... */
#include <linux/bug.h>		/* WARN*() */

#include <linux/kedr_local.h>	/* struct kedr_local */
/* ====================================================================== */

/*
 * May return NULL. The pointer returned by this function may only be passed
 * to other thunks and handlers but should not be dereferenced by the
 * instrumented code itself.
 *
 * [NB] We allocate a kedr_local instance here rather than in a handler to
 * avoid problems when the handlers are detached in runtime (we would still
 * need to free that instance somehow then). This is easier to implement.
 *
 * 'notrace' is used for the thunks to make the code a bit smaller. Ftrace
 * should not be used for these functions anyway, so there is no need to
 * place Ftrace hooks there either.
 */
void __used notrace *kedr_thunk_fentry(void)
{
	struct kedr_local *local = kzalloc(sizeof(*local), GFP_ATOMIC);
	WARN_ON_ONCE(!local);
	return local;
}

void __used notrace kedr_thunk_fexit(struct kedr_local *local)
{
	kfree(local);
}
/* ====================================================================== */

/*
 * Hander stubs
 * 
 * 'noinline' and 'asm volatile ("")' are here to prevent the compiler from
 * optimizing away the calls to these functions.
 */
#define KEDR_DEFINE_STUB(_name) \
void __used noinline kedr_stub_ ## _name (struct kedr_local *local) \
{ \
	(void)local; \
	asm volatile (""); \
}

/* alloc_* - memory allocation event,
 * free_* - memory deallocation event.
 *
 * What is guaranteed:
 *
 * for all handlers:
 * - local is the address of a kedr_local instance, never NULL;
 * - local->pc is meaningful.
 * - If a thunk for a pre-handler sets 'addr', 'size', 'event' or 'pc'
 *   field of kedr_local instance, these fields will not change until
 *   the post-handler runs. That is, unless the pre-handler changes these.
 *
 * alloc_pre:
 * - local->size is the requested size of the memory block (> 0).
 *
 * alloc_post:
 * - called only if the allocation succeeds;
 * - local->size is the requested size of the memory block;
 * - local->addr is the address of the allocated memory block.
 *
 * free_pre:
 * - local->addr is the address of the memory block to be freed (non-NULL,
 *   not a ZERO_SIZE_PTR either).
 * ---------------------
 *
 * Note that it is not guaranteed that if a pre-handler for some function
 * has run, the post-handler will run as well. Likewise, the post-handler
 * may not assume that if it runs, the pre-handler has run before it.
 * The handlers may be attached and detached at any moment.
 */
KEDR_DEFINE_STUB(alloc_pre)
KEDR_DEFINE_STUB(alloc_post)
KEDR_DEFINE_STUB(free_pre)
KEDR_DEFINE_STUB(free_post)
/* ====================================================================== */

void notrace kedr_thunk_kmalloc_pre(unsigned long size,
				    struct kedr_local *local)
{
	if (!local)
		return;

	/*
	 * Set these fields even if 'size' is 0: the thunk for the post
	 * handler may need them to decide if that handler should be called.
	 */
	local->pc = (unsigned long)__builtin_return_address(0);
	local->size = size;

	if (size == 0)
		return;

	kedr_stub_alloc_pre(local);
}

void notrace kedr_thunk_kmalloc_post(unsigned long ret,
				     struct kedr_local *local)
{
	if (!local)
		return;

	if (local->size == 0 || ZERO_OR_NULL_PTR((void *)ret))
		return;

	/* local->pc must have been set by the thunk for the pre-handler. */
	local->addr = ret;
	kedr_stub_alloc_post(local);
}

void notrace kedr_thunk_kfree_pre(unsigned long ptr,
				  struct kedr_local *local)
{
	if (!local)
		return;

	local->pc = (unsigned long)__builtin_return_address(0);
	local->addr = ptr;

	if (ZERO_OR_NULL_PTR((void *)ptr))
		return;

	kedr_stub_free_pre(local);
}

void notrace kedr_thunk_kfree_post(struct kedr_local *local)
{
	if (!local)
		return;

	if (ZERO_OR_NULL_PTR((void *)local->addr))
		return;

	kedr_stub_free_post(local);
}

void notrace kedr_thunk_kmc_alloc_pre(unsigned long kmem_cache,
				      struct kedr_local *local)
{
	struct kmem_cache *kmc = (struct kmem_cache *)kmem_cache;

	if (!local)
		return;

	local->pc = (unsigned long)__builtin_return_address(0);
	if (kmc)
		local->size = (unsigned long)kmem_cache_size(kmc);
	else
		local->size = 0;

	if (local->size == 0)
		return;

	kedr_stub_alloc_pre(local);
}

void notrace kedr_thunk_kmc_alloc_post(unsigned long ret,
				       struct kedr_local *local)
{
	if (!local)
		return;

	if (local->size == 0 || ZERO_OR_NULL_PTR((void *)ret))
		return;

	local->addr = ret;
	kedr_stub_alloc_post(local);
}

void notrace kedr_thunk_kmc_free_pre(unsigned long kmem_cache,
				     unsigned long ptr,
				     struct kedr_local *local)
{
	(void)kmem_cache;

	if (!local)
		return;

	local->pc = (unsigned long)__builtin_return_address(0);
	local->addr = ptr;

	if (ZERO_OR_NULL_PTR((void *)ptr))
		return;

	kedr_stub_free_pre(local);
}

void notrace kedr_thunk_kmc_free_post(struct kedr_local *local)
{
	if (!local)
		return;

	if (ZERO_OR_NULL_PTR((void *)local->addr))
		return;

	kedr_stub_free_post(local);
}
