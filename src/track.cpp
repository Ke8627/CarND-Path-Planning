#include "track.h"

Track::Track(string map_file) {
    vector<double> waypoints_x;
    vector<double> waypoints_y;
    vector<double> waypoints_s;
    vector<double> waypoints_dx;
    vector<double> waypoints_dy;

    ifstream in_map(map_file.c_str(), ifstream::in);

    string line;
    while (getline(in_map, line)) {
        istringstream iss(line);
        double x;
        double y;
        float s;
        float d_x;
        float d_y;
        iss >> x;
        iss >> y;
        iss >> s;
        iss >> d_x;
        iss >> d_y;
        waypoints_x.push_back(x);
        waypoints_y.push_back(y);
        waypoints_s.push_back(s);
        waypoints_dx.push_back(d_x);
        waypoints_dy.push_back(d_y);
    }

    // Splines to support conversion from s,d to x,y.
    // Other direction is also possible but more difficult.
    s_x.set_points(waypoints_s,waypoints_x);
    s_y.set_points(waypoints_s,waypoints_y);
    s_dx.set_points(waypoints_s,waypoints_dx);
    s_dy.set_points(waypoints_s,waypoints_dy);

    // Connect the beginning and end of track if endpoints are close enough.
    min_waypoint_s = waypoints_s[0];
    max_waypoint_s = waypoints_s[waypoints_s.size()-1];
    double endpoint_distance_x = waypoints_x[0] - waypoints_x[waypoints_x.size()-1];
    double endpoint_distance_y = waypoints_y[0] - waypoints_y[waypoints_y.size()-1];
    endpoint_distance = sqrt(endpoint_distance_x*endpoint_distance_x + endpoint_distance_y*endpoint_distance_y);
    circular_track = (endpoint_distance < 100);
}

Track::Track(const Track &track) {
    this->circular_track = track.circular_track;
    this->endpoint_distance = track.endpoint_distance;
    this->max_waypoint_s = track.max_waypoint_s;
    this->min_waypoint_s = track.min_waypoint_s;
    this->s_x = track.s_x;
    this->s_y = track.s_y;
    this->s_dx = track.s_dx;
    this->s_dy = track.s_dy;
}

Track::~Track() {}

vector<double> Track::sd_to_xy(double s, double d) {
    double path_x = s_x(s);
    double path_y = s_y(s);
    double dx = s_dx(s);
    double dy = s_dy(s);
    double x = path_x + d * dx;
    double y = path_y + d * dy;
    return {x,y};
}

vector<double> Track::xy_to_sd(double x, double y) {
    double s_est = min_waypoint_s;
    double dist2_est = pow(s_x(s_est) - x, 2) + pow(s_y(s_est) - y, 2);
    for(int i = 1; i <= 20; i++) {
        double s = i * (max_waypoint_s - min_waypoint_s) / 20 + min_waypoint_s;
        double dist2 = pow(s_x(s) - x, 2) + pow(s_y(s) - y, 2);
        if(dist2 < dist2_est) {
            dist2_est = dist2;
            s_est = s;
        }
    }
    for(int i = 0; i < 20; i++) {
        double x_est = s_x(s_est);
        double y_est = s_y(s_est);
        double x_diff = x - x_est;
        double y_diff = y - y_est;
        double dx = s_dx(s_est);
        double dy = s_dy(s_est);
        // plus_s is unit vector in direction of +s. Rotated 90 degrees counterclockwise from the (dx,dy) vector.
        double plus_s_x = 0 - dy;
        double plus_s_y = dx;
        double plus_s_mag = sqrt(pow(plus_s_x,2)+pow(plus_s_y,2));
        plus_s_x = plus_s_x / plus_s_mag;
        plus_s_y = plus_s_y / plus_s_mag;
        double delta_s = x_diff * plus_s_x + y_diff * plus_s_y;
        s_est += delta_s;
        if(abs(delta_s) < 0.01) {
            break;
        }
    }
    double x_est = s_x(s_est);
    double y_est = s_y(s_est);
    double x_diff = x - x_est;
    double y_diff = y - y_est;
    double dx = s_dx(s_est);
    double dy = s_dy(s_est);
    double d_mag = sqrt(pow(dx,2)+pow(dy,2));
    dx = dx / d_mag;
    dy = dy / d_mag;
    double d_est = dx * x_diff + dy * y_diff;
    return {s_est,d_est};
}
