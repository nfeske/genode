#ifndef ___VBox_com_listeners_h
#define ___VBox_com_listeners_h

#include <VBox/com/com.h>
#include <VBox/com/VirtualBox.h>

#define VBOX_LISTENER_DECLARE(klazz)

#include <base/log.h>

template <class T, class TParam = void *>
struct ListenerImpl
:
	VBOX_SCRIPTABLE_IMPL(IEventListener)
{
	T *mListener = nullptr;

	class Not_implemented { };

	ListenerImpl() { }

	virtual ~ListenerImpl() { }

	HRESULT init(T* aListener, TParam param)
	{
		mListener = aListener;
		return mListener->init(param);
	}

	HRESULT init(T* aListener)
	{
		mListener = aListener;
		return mListener->init();
	}

	void uninit()
	{
		if (mListener)
		{
			mListener->uninit();
			delete mListener;
			mListener = 0;
		}
	}

	HRESULT FinalConstruct() { return S_OK; }

	void FinalRelease() { uninit(); }

	T* getWrapped() { return mListener; }

	NS_IMETHOD_(nsrefcnt) AddRef(void)
	{
		Genode::error(__PRETTY_FUNCTION__, " not implemented");
		throw Not_implemented();
	}

	STDMETHOD(HandleEvent)(IEvent * aEvent) override
	{
		Genode::error(__PRETTY_FUNCTION__, " not implemented");
		throw Not_implemented();
	}
};

#endif /* ___VBox_com_listeners_h */
