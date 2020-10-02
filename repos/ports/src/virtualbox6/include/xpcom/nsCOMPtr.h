#ifndef __gen_nsCOMPtr_h__
#define __gen_nsCOMPtr_h__

#include "nscore.h"
#include "nsID.h"

struct nsCOMPtr_helper
{
	virtual nsresult operator()(char const *nsIID, void**) const = 0;
};


/*
 * Define name as strings for the types used as nsCOMPtr arguments
 *
 * The names are passed as nsIID to the 'nsCOMPtr_helper' interface, see
 * the implementaton of 'do_GetService'.
 */

template <typename T> struct ns_type_trait;

#define NS_TYPE_NAME(Type) \
  struct Type; \
  template <> struct ns_type_trait<Type> { \
    static char const *name() { return #Type; }; \
  };

NS_TYPE_NAME(nsIExceptionManager);
NS_TYPE_NAME(nsIExceptionService);


template <class T>
struct nsCOMPtr
{
	T *_raw_ptr = nullptr;

	static char const *_type_name(nsIExceptionManager const &) { return "nsIExceptionManager"; }

	nsCOMPtr() { };
	nsCOMPtr(nsCOMPtr_helper const &helper)
	{
		helper(ns_type_trait<T>::name(), &_raw_ptr);
	};

	nsCOMPtr<T>& operator = (const nsCOMPtr_helper& helper)
	{
		Genode::log("nsCOMPtr operator = called");
		helper(ns_type_trait<T>::name(), (void **)&_raw_ptr);
		return *this;
	}

	T * operator -> ()
	{
		Genode::log(__PRETTY_FUNCTION__, " called type=", ns_type_trait<T>::name(), " ptr=", _raw_ptr);
		return _raw_ptr;
	}
};


template <class T>
struct already_AddRefed
{
	already_AddRefed(T* aRawPtr) : mRawPtr(aRawPtr) { }

	T* get() const { return mRawPtr; }

	T* mRawPtr;
};


template <class T>
inline const already_AddRefed<T> getter_AddRefs(T* aRawPtr)
{
	return already_AddRefed<T>(aRawPtr);
}


template <class T>
inline already_AddRefed<T> getter_AddRefs( nsCOMPtr<T>& aSmartPtr )
{
	Genode::log(__PRETTY_FUNCTION__, " called");
	return already_AddRefed<T>(aSmartPtr._raw_ptr);
}

#endif /* __gen_nsCOMPtr_h__ */
