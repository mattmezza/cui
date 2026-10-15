/* Keep this first-include sequence identical to README's header-only example. */
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_BASE_IMPLEMENTATION
#include "cui_base.h"
#define CUI_COMPONENTS_IMPLEMENTATION
#include "cui_components.h"

/* Re-including implementation headers must also remain harmless. */
#include "consumer.c"
#include "cui_components.h"
