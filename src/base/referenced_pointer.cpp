
#include <bg2e/base/referenced_pointer.hpp>

#include <iostream>

#define DEBUG_MEMORY_LEAKS 1

namespace bg2e {
namespace base {


	class ReferencedPointerManager {
	public:
#if DEBUG_MEMORY_LEAKS==1
		ReferencedPointerManager() {

		}
#else
		ReferencedPointerManager() :_references(0) {

		}
#endif

		virtual ~ReferencedPointerManager() {
			checkReferences();
		}

#if DEBUG_MEMORY_LEAKS==1
		void addReference(ReferencedPointer* ptr) {
			_references.push_back(ptr);
		}

		void removeReference(ReferencedPointer* ptr) {
			std::vector<ReferencedPointer*>::iterator it = std::find(_references.begin(), _references.end(), ptr);
			if (it != _references.end()) {
				_references.erase(it);
			}
		}
#else
		void addReference(ReferencedPointer* ptr) {
			++_references;
		}

		void removeReference(ReferencedPointer* ptr) {
			--_references;
		}
#endif

		void checkReferences() {

#if DEBUG_MEMORY_LEAKS==1
			if (_references.size() != 0) std::cerr << "WARNING: Possible memory leak. " << _references.size() << " undeleted references found." << std::endl;
#else
			if (_references > 0) std::cerr << "WARNING: Possible memory leak. " << _references << " undeleted references found." << std::endl;
#endif
		}

	protected:
#if DEBUG_MEMORY_LEAKS==1
		std::vector<ReferencedPointer*> _references;
#else
		int _references;
#endif
	};

#if DEBUG_MEMORY_LEAKS==1
	static ReferencedPointerManager s_referencedPointerManager;
#endif

	ReferencedPointer::ReferencedPointer() :_ref(0) {
#if DEBUG_MEMORY_LEAKS==1
		s_referencedPointerManager.addReference(this);
#endif
	}


	ReferencedPointer::~ReferencedPointer() {
#if DEBUG_MEMORY_LEAKS==1
		s_referencedPointerManager.removeReference(this);
#endif
	}

	void ReferencedPointer::inc_ref() {
		++_ref;
	}

	void ReferencedPointer::dec_ref() {
		--_ref;
		if (_ref <= 0) delete this;
	}

	void ReferencedPointer::dec_ref_nodelete() {
		--_ref;
	}

}
}
