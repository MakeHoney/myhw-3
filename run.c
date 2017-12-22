#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include "run.h"
#include "util.h"

void* base = 0;

p_meta find_meta(p_meta *last, size_t size) {
  p_meta index = base;
  p_meta result = base;

  switch(fit_flag){
    case FIRST_FIT:
    {
      p_meta floor = base;

	  while(index && !(index->free && index->size >= size)) {
		  *last = index;
		  index = index->next;
	  }

	  result = index;
    }
    break;

    case BEST_FIT:
    {
      size_t cvtSize = (index->size - size);
	  while(index && (index->free && index->size >= size)){
		  cvtSize = (index->size - size);
		  if(cvtSize < result->size - size)
			  result = index;

		  *last = index;
	  }
	  *last = index;
	  index = index->next;
    }

    break;

    case WORST_FIT:
    {
		size_t cvtSize = (index->size - size);
		while(index && (index->free && index->size >= size)){
			cvtSize = (index->size - size);
			if(cvtSize > result->size - size)
				result = index;
			*last = index;
			index = index->next;
		}
    }
    break;

  }
  return result;
}

void* m_malloc(size_t size) {
	p_meta floor, last;

	size_t cvtSize = ((size - 1) / 4) * 4 + 4;

	if(base) {
		last = base;
		floor = find_meta(&last, cvtSize);

		if(floor) { // 함수호출이 두번 이상일 때
			if((floor->size - cvtSize) >= (META_SIZE + 4))
				splitMeta(floor, cvtSize);
			floor->free = 0;
		}
		else { // split할 공간이 없을 때
			floor = extendHeap(last, cvtSize);

			if(!floor)
				return NULL;
		}
	}
	else {	// 함수호출이 처음일 때
		floor = extendHeap(NULL, cvtSize);

		if(!floor)
			return NULL;
		base = floor;
	}
	return floor->data;
}

void m_free(void *ptr) {
	p_meta floor;

	if(validAddr(ptr)){
		floor = getBlock(ptr);
		floor->free = 1;
	// prev와 통합이 가능하면 통합	
		if(floor->prev && floor->prev->free)
			floor = fusion(floor->prev);

		if(floor->next)
			fusion(floor);
		else {
			if(floor->prev)
				floor->prev->next = NULL;
			else
				base = NULL;
			brk(floor);
		}
	}
}

void* m_realloc(void* ptr, size_t size)
{
	size_t cvtSize = ((size - 1) / 4) * 4 + 4;
	p_meta floor = getBlock(ptr);
	p_meta new;
	void* newp;

	if(!ptr)
		return m_malloc(size);

	if(validAddr(ptr)){
		if(floor->size >= cvtSize){
			if(floor->size - cvtSize >= (META_SIZE + 4))
				splitMeta(floor, cvtSize);
		}
		else {
			if(floor->next && floor->next->free
					&&(floor->size + META_SIZE + floor->next->size)
						>= cvtSize){
				fusion(floor);
				if(floor->size - cvtSize >= (META_SIZE + 4))
					splitMeta(floor, cvtSize);
			}
			else{
				newp = m_malloc(cvtSize);
				if(!newp)
					return NULL;
				new = getBlock(newp);
				copy_block(floor, new);
				m_free(ptr);
				return newp;
			}
		}
		return ptr;
	}
	return NULL;
}

p_meta extendHeap(p_meta last, size_t size){
	p_meta floor = sbrk(0);
	int sb = (int)sbrk(META_SIZE + size);

	if(sb < 0)
		return NULL;

	floor->size = size;
	floor->next = NULL;
	floor->prev = last;
	floor->ptr = floor->data;

	if(last)
		last->next = floor;
	floor->free = 0;

	return floor;
}

void splitMeta(p_meta floor, size_t size){
	p_meta new;
	new = (p_meta)floor->data + size;
	new->size = (floor->size) - size - META_SIZE;
	new->next = floor->next;
	new->prev = floor;
	new->free = 1;
	new->ptr = new->data;
	floor->size = size;
	floor->next = new;
	if(new->next)
		new->next->prev = new;
}

p_meta fusion(p_meta floor){
	if(floor->next && floor->next->free){
		floor->size += META_SIZE + floor->next->size;
		// free할 블록의 사이즈 반환
		floor->next = floor->next->next;
		// free할 블록의 다음 블록과 free이전 블록 연결
		
		if(floor->next)
			floor->next->prev = floor;
		// floor의 다음 블록이 null이 아니면 해당 블록의
		// prev를 floor로 연결시켜줌
	}

	return floor;
}

p_meta getBlock(void* p){
	char* tmp;
	tmp = p;
	return (p = tmp -= META_SIZE);
}

int validAddr(void* p){
	if(base)
		if(p > base && p < sbrk(0))
			return (p == (getBlock(p))->ptr);
	return 0;
}

void copy_block(p_meta src, p_meta dst){
	int *sdata, *ddata;
	size_t i;
	sdata = src->ptr;
	ddata = dst->ptr;
	for(i = 0 ; i*4 <src->size && i * 4 < dst->size; i++)
		ddata[i] = sdata[i];
}
