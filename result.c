/*

   maxxc - maximise cross country flights
   Copyright (C) 2008  Tom Payne

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "maxxc.h"

    void
route_push_wpt(route_t *route, const wpt_t *wpt)
{
    if (route->nwpts == route->wpts_capacity) {
	route->wpts_capacity = route->wpts_capacity ? 2 * route->wpts_capacity : 8;
	route->wpts = realloc(route->wpts, route->wpts_capacity * sizeof(wpt_t));
	if (!route->wpts)
	    DIE("realloc", errno);
    }
    route->wpts[route->nwpts] = *wpt;
    ++route->nwpts;
}

    void
route_push_trkpts(route_t *route, const trkpt_t *trkpts, int n, int *indexes, const char **names)
{
    for (int i = 0; i < n; ++i) {
	wpt_t wpt;
	trkpt_to_wpt(trkpts + indexes[i], &wpt);
	wpt.name = (char *) names[i];
	route_push_wpt(route, &wpt);
    }
}

    result_t *
result_new(const char *league)
{
    result_t *result = alloc(sizeof(result_t));
    result->league = league;
    return result;
}

    void
result_delete(result_t *result)
{
    if (result) {
	for (int i = 0; i < result->nroutes; ++i)
	    free(result->routes[i].wpts);
	free(result->routes);
	free(result);
    }
}

    route_t *
result_push_new_route(result_t *result, const char *name, double distance, double multiplier, int circuit, int declared)
{
    if (result->nroutes == result->routes_capacity) {
	result->routes_capacity = result->routes_capacity ? 2 * result->routes_capacity : 8;
	result->routes = realloc(result->routes, result->routes_capacity * sizeof(route_t));
	if (!result->routes)
	    DIE("realloc", errno);
    }
    route_t *route = result->routes + result->nroutes++;
    memset(route, 0, sizeof(route_t));
    route->name = name;
    route->distance = distance;
    route->multiplier = multiplier;
    route->circuit = circuit;
    route->declared = declared;
    return route;
}

    static void
time_write_gpx(time_t time, FILE *file, const char *prefix)
{
    if (time != (time_t) -1) {
	struct tm tm;
	if (!gmtime_r(&time, &tm))
	    DIE("gmtime_r", errno);
	char time[32];
	if (!strftime(time, sizeof time, "%Y-%m-%dT%H:%M:%SZ", &tm))
	    DIE("strftime", errno);
	fprintf(file, "%s<time>%s</time>\n", prefix, time);
    }
}

    static void
wpt_write_gpx(const wpt_t *wpt, FILE *file, const char *type)
{
    fprintf(file, "\t\t<%s lat=\"%.8f\" lon=\"%.8f\">\n", type, wpt->lat / 60000.0, wpt->lon / 60000.0);
    if (wpt->val == 'A')
	fprintf(file, "\t\t\t<ele>%d</ele>\n", wpt->ele);
    time_write_gpx(wpt->time, file, "\t\t\t");
    if (wpt->name)
	fprintf(file, "\t\t\t<name>%s</name>\n", wpt->name);
    fprintf(file, "\t\t\t<fix>%s</fix>\n", wpt->val == 'A' ? "3d" : "2d");
    fprintf(file, "\t\t</%s>\n", type);
}

    static void
route_write_gpx(const route_t *route, FILE *file)
{
    fprintf(file, "\t<rte>\n");
    if (route->name)
	fprintf(file, "\t\t<name>%s</name>\n", route->name);
    fprintf(file, "\t\t<extensions>\n");
    fprintf(file, "\t\t\t<distance>%.3f</distance>\n", route->distance);
    fprintf(file, "\t\t\t<multiplier>%.1f</multiplier>\n", route->multiplier);
    fprintf(file, "\t\t\t<score>%.2f</score>\n", route->distance * route->multiplier);
    if (route->circuit)
	fprintf(file, "\t\t\t<circuit/>\n");
    if (route->declared)
	fprintf(file, "\t\t\t<declared/>\n");
    fprintf(file, "\t\t</extensions>\n");
    for (int i = 0; i < route->nwpts; ++i)
	wpt_write_gpx(route->wpts + i, file, "rtept");
    fprintf(file, "\t</rte>\n");
}

    static void
trkpt_write_gpx(const trkpt_t *trkpt, FILE *file)
{
    fprintf(file, "\t\t\t<trkpt lat=\"%.8f\" lon=\"%.8f\">\n", trkpt->lat / 60000.0, trkpt->lon / 60000.0);
    if (trkpt->val == 'A')
	fprintf(file, "\t\t\t\t<ele>%d</ele>\n", trkpt->ele);
    time_write_gpx(trkpt->time, file, "\t\t\t\t");
    fprintf(file, "\t\t\t\t<fix>%s</fix>\n", trkpt->val == 'A' ? "3d" : "2d");
    fprintf(file, "\t\t\t</trkpt>\n");
}

    static void
track_write_gpx(const track_t *track, FILE *file)
{
    fprintf(file, "\t<trk>\n");
    fprintf(file, "\t\t<trkseg>\n");
    for (int i = 0; i < track->ntrkpts; ++i)
	trkpt_write_gpx(track->trkpts + i, file);
    fprintf(file, "\t\t</trkseg>\n");
    fprintf(file, "\t</trk>\n");
}

    void
result_write_gpx(const result_t *result, const track_t *track, FILE *file)
{
    fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(file, "<gpx version=\"1.1\" creator=\"http://code.google.com/p/maxxc/\">\n");
    fprintf(file, "\t<metadata>\n");
    fprintf(file, "\t\t<extensions>\n");
    fprintf(file, "\t\t\t<league>%s</league>\n", result->league);
    fprintf(file, "\t\t</extensions>\n");
    fprintf(file, "\t</metadata>\n");
    for (int i = 0; i < result->nroutes; ++i)
	route_write_gpx(result->routes + i, file);
    if (track)
	track_write_gpx(track, file);
    fprintf(file, "</gpx>\n");
}
