#ifndef ___VBox_com_ptr_h
#define ___VBox_com_ptr_h

#include <VBox/com/defs.h>

#include <base/log.h>

template <typename T>
struct ComPtr
{
	T *m_p;

	ComPtr<T> () : m_p(nullptr) { }

	/* copy constructor */
	ComPtr<T> (T *obj) : m_p(obj) { }

	template <class T2>
	ComPtr<T>(const ComPtr<T2> &that) : ComPtr<T>((T2*)that) { }

	template <class T2>
	ComPtr(T2 *p)
	{
		m_p = NULL;
		if (p)
			p->QueryInterface(COM_IIDOF(T), (void **)&m_p);
	}

	/* operators */
	T * operator->() const  { return m_p; }

	operator T*() const { return m_p; }

	template <class T2>
	ComPtr& operator=(const ComPtr<T2> &that)
	{
		return operator=((T2*)that);
	}

	ComPtr& operator=(const ComPtr &that)
	{
		return operator=((T*)that);
	}

	ComPtr& operator=(T *p)
	{
		m_p = p;
		return *this;
	}

	bool isNull () const    { return m_p == nullptr; }
	bool isNotNull() const  { return m_p != nullptr; }

	T ** asOutParam() { return &m_p; }

	template <class T2>
	HRESULT queryInterfaceTo(T2 **pp) const {
		if (pp == nullptr)
			return E_INVALIDARG;

		*pp = static_cast<T2*>(m_p);
		return S_OK;
	}

	void setNull() { m_p = nullptr; }

	template <class T2>
	bool operator==(T2* p)
	{
		return (p == m_p);
	}
};


template <class T>
struct ComObjPtr : ComPtr<T>
{
	ComObjPtr<T> () : ComPtr<T>() { }

	/* copy constructor */
	ComObjPtr<T> (T *obj) : ComPtr<T>(obj) { }

	ComObjPtr& operator=(T *that_p)
	{
		ComPtr<T>::operator=(that_p);
		return *this;
	}

	HRESULT createObject()
	{
		T * obj = new T { };
		if (!obj)
			return E_OUTOFMEMORY;

		ComPtr<T>::m_p = obj;
		return obj->FinalConstruct();
	}
};

#endif /* ___VBox_com_ptr_h */
