#include "../xps.h"
//here, some extension-mime types pairs are given, you can add as required
xps_keyval_t mime_types[] = {
    {".c", "text/x-c"},
    {".cc", "text/x-c"},
    {".cpp", "text/x-c"},
    {".dir", "application/x-director"},
    {".dxr", "application/x-director"},
    {".fgd", "application/x-director"},
    {".swa", "application/x-director"},
    {".text", "text/plain"},
    {".txt", "text/plain"},
    {".png", "image/png"},
    {".png", "image/x-png"},
    };
int n_mimes = sizeof(mime_types) / sizeof(mime_types[0]);

const char *xps_get_mime(const char *file_path){
    const char *text = get_file_text(file_path);

    if(text==NULL){
        return NULL;
    }

    for(int i = 0; i<n_mimes; i++){
        if(strcmp(mime_types[i].key, text)==0){
            return mime_types[i].val;
        }
    }

    return NULL;
}