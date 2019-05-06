//
//  HTML PAGE
//
#include <string.h>

void send_status_html()
{
    char *PAGE_Status = (char*) malloc(2048);
    char *tstr = (char*) malloc(512);
    strcpy(PAGE_Status,"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />");
    strcat(PAGE_Status,"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
    strcat(PAGE_Status,"<a href=\"/\"  class=\"btn btn--s\"><</a>&nbsp;&nbsp;<strong>Status</strong>");
    strcat(PAGE_Status,"<hr>");
    strcat(PAGE_Status,"<style>"); 
    strcat(PAGE_Status,"table, th, td {");
    strcat(PAGE_Status,"  border: 1px solid black;");
    strcat(PAGE_Status,"  border-collapse: collapse;");
    strcat(PAGE_Status,"}");
    strcat(PAGE_Status,"</style>");
    strcat(PAGE_Status,"<table>");
    sprintf(tstr,"<tr><td>DateTime</td><td>%02d/%02d/%02d %02d:%02d:%02d %02d</td></tr>",DateTime.month,DateTime.day,DateTime.year,DateTime.hour,DateTime.minute,DateTime.second,config.timeZone);
    strcat(PAGE_Status,tstr);
    sprintf(tstr,"<tr><td>suntime</td><td>Rise %02d:%02d Set %02d:%02d</td></tr>",suntime.riseHour,suntime.riseMin,suntime.setHour,suntime.setMin);
    strcat(PAGE_Status,tstr);
    uint32_t freem = system_get_free_heap_size();
    sprintf(tstr,"<tr><td>Free Memory</td><td>%d</td><td>",freem);
    strcat(PAGE_Status,tstr);
    sprintf(tstr,"<tr><td>Relay 1</td><td>1 on %02d:%02d 1 off %02d:%02d 2 on %02d:%02d 2 off %02d:%02d</td></tr>",config.RSchedule[0][0].onHour,config.RSchedule[0][0].onMin,config.RSchedule[0][0].offHour,config.RSchedule[0][0].offMin,config.RSchedule[0][1].onHour,config.RSchedule[0][1].onMin,config.RSchedule[0][1].offHour,config.RSchedule[0][1].offMin);
    strcat(PAGE_Status,tstr);
    sprintf(tstr,"<tr><td>Relay 1</td><td>1 on %02d:%02d 1 off %02d:%02d 2 on %02d:%02d 2 off %02d:%02d</td></tr>",config.RSchedule[1][0].onHour,config.RSchedule[1][0].onMin,config.RSchedule[1][0].offHour,config.RSchedule[1][0].offMin,config.RSchedule[1][1].onHour,config.RSchedule[1][1].onMin,config.RSchedule[1][1].offHour,config.RSchedule[1][1].offMin);
    strcat(PAGE_Status,tstr);
    strcat(PAGE_Status,"</table>"); 
    server.send ( 200, "text/html", PAGE_Status );
    free(PAGE_Status);
    free(tstr);
	Serial.println(__FUNCTION__); 
}

