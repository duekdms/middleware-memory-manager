﻿#pragma once

#include "IMemoryManager.h"
#include "PageIndex.h"
#include "Exception.h"

/**
   *
   * @brief Memory Manager
   * @date 2024-05-02
   * @version 0.21
   *
   */
class MemoryManager : public IMemoryManager {
private:

	char* pBuffer;
	size_t sizeBuffer;
	size_t sizePage;

	int numPages;
	PageIndex* aPageIndex;

	size_t normalizeSize(size_t size) {
		size_t size1 = size >> 4;
		size_t size2 = size1 << 4;
		size = ((size == size2) ? size1 : ++size1) << 4;
		return size;
	}
	PageIndex* allocateNewPages(size_t sizeSlot) {
		int numPagesRequired = (sizeSlot % sizePage) == 0 ? (int)(sizeSlot / sizePage) : (int)(sizeSlot / sizePage) + 1;

		PageIndex* pPageIndexAllocated = nullptr;
		bool bFound = false;
		int numConsequtivePages = 0;

		for (int i = numPages - 1; i >= 0; i--) {
			if (bFound) {
				if (this->aPageIndex[i].isAllocated()) {
					break;
				}
				else {
					numConsequtivePages++;
					this->aPageIndex[i].setNumConsecutivePages(numConsequtivePages);
				}
			}
			else {
				if (!this->aPageIndex[i].isAllocated()) {
					if (this->aPageIndex[i].getNumConsecutivePages() == numPagesRequired) {
						pPageIndexAllocated = &(this->aPageIndex[i]);
						for (int k = i; k < i + numPagesRequired; k++) {
							this->aPageIndex[k].initialize(sizeSlot);
							this->aPageIndex[k].setBAllocated(true);
						}
						bFound = true;
					}
				}
			}
		}
		return pPageIndexAllocated;
	}

	void collectGarbage() {

	}

public:
	void* operator new(size_t size) {
		return malloc(size);
	}
	void operator delete(void* pObject) {
		free(pObject);
	}

	MemoryManager(char* pBuffer, size_t sizeBuffer, size_t sizePage) :
		pBuffer(pBuffer),
		sizeBuffer(sizeBuffer),
		sizePage(sizePage)
	{
		numPages = (int)(sizeBuffer / sizePage);
		aPageIndex = new PageIndex[numPages];

		char* pBufferCurrent = pBuffer;
		for (int i = 0; i < numPages; i++) {
			aPageIndex[i].setPPage(pBufferCurrent);
			aPageIndex[i].setSizePage(sizePage);
			aPageIndex[i].setNumConsecutivePages(numPages - i);
			pBufferCurrent = pBufferCurrent + sizePage;
		}
	}
	virtual ~MemoryManager() {
	}


	void* allocate(size_t sizeMemory, const char* pName) {
		// multiple x
		size_t sizeSlot = normalizeSize(sizeMemory);

		// search for allocated PageIndex s
		PageIndex* pPageIndexFound = nullptr;
		for (int i = 0; i < numPages; i++) {
			if (this->aPageIndex[i].isAllocated()) {
				if (this->aPageIndex[i].getSizeSlot() == sizeSlot) {
					// found
					pPageIndexFound = &(this->aPageIndex[i]);
					break;
				}
			}
		}

		// not found
		if (pPageIndexFound == nullptr) {
			pPageIndexFound = allocateNewPages(sizeSlot);
			if (pPageIndexFound == nullptr) {
				// out of memory
				throw Exception(Exception::ECode::eOutOfMemory, "allocateNewPages1");
			}
		}

		// allocate a slot
		void* pSlotAllocated = pPageIndexFound->allocate(sizeSlot, pName);
		// no more slots
		if (pSlotAllocated == nullptr) {
			pPageIndexFound = allocateNewPages(sizeSlot);
			if (pPageIndexFound == nullptr) {
				// out of memory
				throw Exception(Exception::ECode::eOutOfMemory, "allocateNewPages2");
			}
			else {
				pSlotAllocated = pPageIndexFound->allocate(sizeSlot, pName);
			}
		}
		return pSlotAllocated;
	}

	void dellocate(void* pObject) {
		size_t offset = (size_t)pObject - (size_t)(this->pBuffer);
		int pageIndex = (int)(offset / sizePage);
		bool isEmpty = this->aPageIndex[pageIndex].dellocate(pObject);
		if (isEmpty) {
			for (int i = 0; i < this->aPageIndex[pageIndex].getNumConsecutivePages(); i++) {
				this->aPageIndex[pageIndex + i].finalize();
			}
		}
		int startConsecutivePages = this->numPages - 1;
		for (int i = pageIndex; i < numPages; i++) {
			if (this->aPageIndex[i].isAllocated()) { 
				startConsecutivePages = i - 1;
				break; 
			}
		}
		int numConsecutivePages = 1;
		for (int i = startConsecutivePages; i >= 0 || this->aPageIndex[pageIndex].isAllocated(); i--) {
			this->aPageIndex[i].setNumConsecutivePages(numConsecutivePages);
			numConsecutivePages++;
		}
		// ===============
		this->collectGarbage();
		// ===============
	}

	SlotIndex* findASlotIndex(void* pObject) {
		size_t offset = (size_t)pObject - (size_t)(this->pBuffer);
		int pageIndex = (int)(offset / sizePage);
		SlotIndex* pSlotIndexFound = this->aPageIndex[pageIndex].findASlotIndex(pObject);
		return pSlotIndexFound;
	}
	PageIndex findAPageIndex(void* pObject) {
		size_t offset = (size_t)pObject - (size_t)(this->pBuffer);
		int pageIndex = (int)(offset / sizePage);
		return this->aPageIndex[pageIndex];
	}

	void showStatus() {
		printf("Start==========================================\n");
		for (int i = 0; i < numPages; i++) {
			for (int j = 0; j < aPageIndex[i].getNumConsecutivePages(); j++) {
				printf("PageIndex%d(SizeSlot=%d, ConsecutivePages=%d)\n", i + j, (int)aPageIndex[i + j].getSizeSlot(), aPageIndex[i + j].getNumConsecutivePages());
			}
			this->aPageIndex[i].showStatus();
			i = i + aPageIndex[i].getNumConsecutivePages() - 1;
		}
		printf("End============================================\n");
	}
};
