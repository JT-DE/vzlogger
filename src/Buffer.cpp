/**
 * Circular buffer (double linked)
 *
 * Used to store recent readings and buffer in case of net inconnectivity
 *
 * @author Steffen Vogel <info@steffenvogel.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "float.h" /* double min max */

#include "Buffer.hpp"

Buffer::Buffer() :
		_keep(32)
{
	_newValues=false;
	pthread_mutex_init(&_mutex, NULL);
	_aggmode=NONE;
}

void Buffer::push(const Reading &rd) {
	lock();
	_sent.push_back(rd);
	unlock();
}

void Buffer::aggregate() {
	if(_aggmode == NONE) return;
	if(_aggmode == MAXIMUM) {
		lock();
		Reading *latest=NULL;
		double aggvalue=DBL_MIN;
		for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if(! it->deleted()) {
				if(!latest) {
					latest=&*it;
				} else {
					if(it->tvtod() > latest->tvtod()) {
						latest=&*it;
					}
				}
				aggvalue=std::max(aggvalue,it->value());
			}
		}
		for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if(! it->deleted()) {
				if(&*it==latest) {
					it->value(aggvalue);
				} else {
					it->mark_delete();
				}
			}
		}
		unlock();
		return;
	}
	if(_aggmode == AVG) {
		lock();
		Reading *latest=NULL;
		double aggvalue=0;
		int aggcount=0;
		for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if(! it->deleted()) {
				if(!latest) {
					latest=&*it;
				} else {
					if(it->tvtod() > latest->tvtod()) {
						latest=&*it;
					}
				}
				aggvalue=aggvalue+it->value();
				aggcount++;
			}
		}
		for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
			if(! it->deleted()) {
				if(&*it==latest) {
					it->value(aggvalue/aggcount);
				} else {
					it->mark_delete();
				}
			}
		}
		unlock();
		return;
	}
}


void Buffer::clean() {
	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		if(it->deleted()) {
			if( it == _sent.begin() ){
				_sent.erase(it);
				it = _sent.begin();
			} else {
				it = _sent.erase(it);
			}
		}

	}
//_sent.clear();
	unlock();
}

void Buffer::undelete() {
	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		it->reset();
	}
	unlock();
}


void Buffer::shrink(/*size_t keep*/) {
	lock();

//	while(size > keep && begin() != sent) {
//		pop();
//	}

	unlock();
}

char * Buffer::dump(char *dump, size_t len) {
	size_t pos = 0;
	dump[pos++] = '{';

	lock();
	for(iterator it = _sent.begin(); it!= _sent.end(); it++) {
		if (pos < len) {
			pos += snprintf(dump+pos, len-pos, "%.4f", it->value());
		}

		/* indicate last sent reading */
		if (pos < len && _sent.end() == it) {
			dump[pos++] = '!';
		}

		/* add seperator between values */
		if (pos < len && it != _sent.end()) {
			dump[pos++] = ',';
		}
	}

	if (pos+1 < len) {
		dump[pos++] = '}';
		dump[pos] = '\0'; /* zero terminated string */
	}
	unlock();
	
	return (pos < len) ? dump : NULL; /* buffer full? */
}

Buffer::~Buffer() {
	pthread_mutex_destroy(&_mutex);
}

/*
 * Local variables:
 *  tab-width: 2
 *  c-indent-level: 2
 *  c-basic-offset: 2
 *  project-name: vzlogger
 * End:
 */