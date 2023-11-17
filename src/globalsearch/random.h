/**********************************************************************
  random.h - Provides a function to generate random doubles/ints between a
             min and a max value

  Copyright (C) 2016 by Patrick S. Avery

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

 ***********************************************************************/

#ifndef GLOBALSEARCH_RANDOM_H
#define GLOBALSEARCH_RANDOM_H

#include <cstdlib>
#include <time.h>

namespace GlobalSearch {

// Robert Jenkins' 96 bit Mix Function
static unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

static void seed_rand_mix(int cseed = -1) {
  unsigned long seed;
  if (cseed >= 0) {
    seed = (unsigned long) cseed;
  } else {
    seed = mix(clock(), time(NULL), getpid());
  }
  srand(seed);
}

static inline double getRandDouble(double min = 0.0, double max = 1.0)
{
  double f = (double)rand() / RAND_MAX;
  return min + f * (max - min);
}

static inline int getRandInt(int min = INT_MIN, int max = INT_MAX)
{
  int f = rand() / RAND_MAX;
  return min + f * (max - min);
}

static inline unsigned int getRandUInt(unsigned int min = 0,
                                       unsigned int max = UINT_MAX)
{
  int f = rand() / RAND_MAX;
  return (unsigned int)(min + f * (max - min));
}
}

#endif
