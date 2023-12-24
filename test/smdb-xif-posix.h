/*    Copyright 2023 Davide Libenzi
 * 
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 * 
 *        http://www.apache.org/licenses/LICENSE-2.0
 * 
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 * 
 */


#ifndef _SMDB_XIF_POSIX_H
#define _SMDB_XIF_POSIX_H

struct smdbxi_file *smdb_xif_file(int fd, int closefd, char const *filename,
				  int flags, int unlinkfile);
struct smdbxi_factory *smdb_xif_factory(void);

#endif
