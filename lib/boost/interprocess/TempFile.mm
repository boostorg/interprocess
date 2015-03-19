#import <Foundation/Foundation.h>
#include <string>

namespace boost {
	namespace interprocess {
		namespace ipcdetail {
			void get_temp_dir(std::string& strPath) {
				@autoreleasepool {
					// Files in NSTemporaryDirectory are automatically deleted after an unspecified time (apparently 3 days)
					// This is ok if a process still holds a file handle to that file. The file system blocks are not
					// freed until the handle is freed.
					// But no other process can access this file via the file name.
					// http://www.cocoawithlove.com/2009/07/temporary-files-and-folders-in-cocoa.html
					NSArray *astrPaths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
					if(0<[astrPaths count]) {
						strPath.assign([[astrPaths objectAtIndex:0] UTF8String]);
					}
				}
			}
		}
	}
}