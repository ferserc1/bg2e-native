
#ifndef _bg2e_base_referenced_pointer_hpp_
#define _bg2e_base_referenced_pointer_hpp_

#include <vector>
#include <algorithm>

namespace bg2e {
namespace base {

	class ReferencedPointer;

	}

	template <class T> T native_cast(void* nativePtr) { return static_cast<T>(nativePtr); }
	typedef void* plain_ptr;

	template <class T>
	class ptr {
	public:
		ptr() :_ptr(nullptr) {}
		ptr(T* p) :_ptr(p) { if (_ptr) _ptr->inc_ref(); }
		ptr(const ptr& p) :_ptr(p._ptr) { if (_ptr) _ptr->inc_ref(); }
		~ptr() { if (_ptr) _ptr->dec_ref(); _ptr = nullptr; }

		ptr& operator=(const ptr& p) {
			assign(p);
			return *this;
		}
		inline ptr& operator=(T* p) {
			if (_ptr == p) {
				return *this;
			}
			T* tmp_ptr = _ptr;
			_ptr = p;
			if (_ptr) _ptr->inc_ref();
			if (tmp_ptr) tmp_ptr->dec_ref();
			return *this;
		}
		bool operator==(const ptr& p) const { return _ptr == p._ptr; }
		bool operator==(const T* p) const { return _ptr == p; }
		friend bool operator==(const T* p, const ptr& rp) { return p == rp._ptr; }

		bool operator < (const ptr& p) const { return _ptr < p._ptr; }

		T& operator*() const { return *_ptr; }
		T* operator->() const { return _ptr; }

		bool operator!() const { return _ptr == nullptr; }
		bool valid() const { return _ptr != nullptr; }
		bool isNull() const { return _ptr == nullptr; }
		T* release() { T* tmp = _ptr; if (_ptr) _ptr->dec_ref_nodelete(); _ptr = nullptr; return tmp; }
		void swap(ptr& p) { T* tmp = _ptr; _ptr = p._ptr; p._ptr = tmp; }

		bool operator==(T* p) const { return _ptr == p; }

		T* getPtr() { return _ptr; }
		const T* getPtr() const { return _ptr; }

		T* operator->() { return _ptr; }

	protected:
		void assign(const ptr<T>& p) {
			if (_ptr == p._ptr) return;
			T* tmp = _ptr;
			_ptr = p._ptr;
			if (_ptr) _ptr->inc_ref();
			if (tmp) tmp->dec_ref();
		}

		T* _ptr;
	};

	namespace base {

		class ReferencedPointer {
		public:
			ReferencedPointer();

			void inc_ref();
			void dec_ref();
			void dec_ref_nodelete();

			inline int getReferences() const { return _ref; }

		protected:
			virtual ~ReferencedPointer();

			int _ref;
		};

}
}

#endif
