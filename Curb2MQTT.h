//Define this to get some extra debugging prints
//
//#define DEBUG_PRINT 1

/*
 *
 * Functions!
 *
 */
// Read Curb2MQTT.config and populate settings
//
extern void read_config();

// Connect to curb authentication server and get an authentication Token
// See https://github.com/Curb-v2/third-party-app-integration/blob/master/docs/api.md
//
extern void get_curb_token();

// Connect curb and get a newly converted websocket
//
extern void create_websocket();

/*
 *
 * API Constants from the Curb API
 *
 */
#define API_HOST L"app.energycurb.com"
#define API_PATH L"/socket.io/?EIO=3&transport=websocket"
#define AUTH_MAX 4096
#define AUTH_HOST L"energycurb.auth0.com"
#define AUTH_PATH  L"/oauth/token"

/*
 *
 * SLOPPY GLOBAL VARIABLES
 *
 */

// Stuff to load from config file
//
extern const char *CURB_USERNAME;
extern const char *CURB_PASSWORD;
extern const char *CURB_CLIENT_ID;
extern const char *CURB_CLIENT_SECRET;

// Stuff to get from authentication server
//
extern char *AUTH_BUF;        // Auth buffer holding auth code from server
extern char *AUTH_CODE;       // Subset of AUTH_BUF with the actual auth code

