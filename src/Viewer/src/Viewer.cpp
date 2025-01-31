/*
 * Copyright (C) 2020 Istituto Italiano di Tecnologia (IIT)
 *
 * This software may be modified and distributed under the terms of the
 * GPL-2+ license. See the accompanying LICENSE file for details.
 */

#include <Viewer.h>

#include <RobotsIO/Camera/Camera.h>
#include <RobotsIO/Camera/CameraParameters.h>
#include <RobotsIO/Camera/RealsenseCameraYarp.h>
#include <RobotsIO/Camera/YarpCamera.h>

#include <RobotsViz/VtkPointCloud.h>

using namespace RobotsIO::Camera;
using namespace RobotsViz;
using namespace yarp::os;


Viewer::Viewer(const ResourceFinder& resource_finder)
{
    const std::string port_prefix = "robots-viz-viewer";
    const bool fps = resource_finder.check("fps", Value(30.0)).asFloat64();

    const Bottle& camera_bottle = resource_finder.findGroup("CAMERA");
    if (camera_bottle.isNull())
        throw(std::runtime_error(log_name_ + "::ctor. Malformed configuration file: cannot find CAMERA section."));

    const std::string camera_source = camera_bottle.check("source", Value("YARP")).asString();

    auto camera_width = camera_bottle.find("width");
    auto camera_height = camera_bottle.find("height");
    auto camera_fx = camera_bottle.find("fx");
    auto camera_fy = camera_bottle.find("fy");
    auto camera_cx = camera_bottle.find("cx");
    auto camera_cy = camera_bottle.find("cy");

    bool valid_camera_values = !(camera_width.isNull()) && camera_width.isInt32();
    valid_camera_values &= !(camera_height.isNull()) && camera_height.isInt32();
    valid_camera_values &= !(camera_fx.isNull()) && camera_fx.isFloat64();
    valid_camera_values &= !(camera_fy.isNull()) && camera_fy.isFloat64();
    valid_camera_values &= !(camera_cx.isNull()) && camera_cx.isFloat64();
    valid_camera_values &= !(camera_cy.isNull()) && camera_cy.isFloat64();

    std::unique_ptr<Camera> camera;
    if (camera_source == "YARP")
    {
        if (!valid_camera_values)
        throw(std::runtime_error(log_name_ + "::ctor. Camera parameters from configuration are invalid."));

        camera = std::unique_ptr<YarpCamera>
        (
            new YarpCamera(camera_width.asInt32(),
                           camera_height.asInt32(),
                           camera_fx.asFloat64(),
                           camera_cx.asFloat64(),
                           camera_fy.asFloat64(),
                           camera_cy.asFloat64(),
                           port_prefix, true)
        );
    }
    else if (camera_source == "RealsenseCamera")
    {
        camera = std::unique_ptr<RealsenseCameraYarp>
        (
            new RealsenseCameraYarp(port_prefix)
        );
    }
    else
        throw(std::runtime_error(log_name_ + "::ctor. Camera " + camera_source + " is not supported."));

    /* Initialize point cloud. */
    const double far_plane = camera_bottle.check("far_plane", Value(10.0)).asFloat64();
    const double subsampling_radius = camera_bottle.check("subsampling_radius", Value(-1)).asFloat64();
    std::unique_ptr<VtkPointCloud> pc = std::unique_ptr<VtkPointCloud>(new VtkPointCloud(std::move(camera), far_plane, subsampling_radius));

    vtk_container_ = std::unique_ptr<VtkContainer>(new VtkContainer(1.0 / fps, 600, 600, false));

    /* Add content to the container. */
    vtk_container_->add_content("point_cloud", std::move(pc));
}


void Viewer::run()
{
    vtk_container_->run();
}
