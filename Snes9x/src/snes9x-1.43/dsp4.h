// debug
int block;								// current block number
extern int c;

// op control
int8 DSP4_Logic;					// controls op flow

// projection format
const int16 PLANE_START = 0x7fff;	// starting distance

int16 view_plane;					// viewer location
int16 far_plane;					// next milestone into screen
int16 segments;						// # raster segments to draw
int16 raster;							// current raster line

int16 project_x;					// current x-position
int16 project_y;					// current y-position

int16 project_centerx;		// x-target of projection
int16 project_centery;		// y-target of projection

int16 project_x1;					// current x-distance
int16 project_x1low;			// lower 16-bits
int16 project_y1;					// current y-distance
int16 project_y1low;			// lower 16-bits

int16 project_x2;					// next projected x-distance
int16 project_y2;					// next projected y-distance

int16 project_pitchx;			// delta center
int16 project_pitchxlow;	// lower 16-bits
int16 project_pitchy;			// delta center
int16 project_pitchylow;	// lower 16-bits

int16 project_focalx;			// x-point of projection at viewer plane
int16 project_focaly;			// y-point of projection at viewer plane

int16 project_ptr;				// data structure pointer

// render window
int16 center_x;						// x-center of viewport
int16 center_y;						// y-center of viewport
int16 viewport_left;			// x-left of viewport
int16 viewport_right;			// x-right of viewport
int16 viewport_top;				// y-top of viewport
int16 viewport_bottom;		// y-bottom of viewport

// sprite structure
int16 sprite_x;						// projected x-pos of sprite
int16 sprite_y;						// projected y-pos of sprite
int16 sprite_offset;			// data pointer offset
int8 sprite_type;					// vehicle, terrain
bool8 sprite_size;				// sprite size: 8x8 or 16x16

// path strips
int16 path_clipRight[4];		// value to clip to for x>b
int16 path_clipLeft[4];			// value to clip to for x<a
int16 path_pos[4];					// x-positions of lanes
int16 path_ptr[4];					// data structure pointers
int16 path_raster[4];				// current raster
int16 path_top[4];					// viewport_top

int16 path_y[2];						// current y-position
int16 path_x[2];						// current focals
int16 path_plane[2];				// previous plane

// op09 window sorting
int16 multi_index1;					// index counter
int16 multi_index2;					// index counter
bool8 op09_mode;						// window mode

// multi-op storage
int16 multi_focaly[64];			// focal_y values
int16 multi_farplane[4];		// farthest drawn distance
int16 multi_raster[4];			// line where track stops

// OAM
int8 op06_OAM[32];					// OAM (size,MSB) data
int8 op06_index;						// index into OAM table
int8 op06_offset;						// offset into OAM table

short MaxTilesPerRow = 0;
short RowCount[32];
