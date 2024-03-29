#include "stdafx.h"
#include "ServicesResource.h"
#include "StringHash.h"
#include "Application.h"
#include "Version.h"
#include "Subsystems.h"
#include <AB/Entities/ServiceList.h>
#include <AB/Entities/Service.h>
#include "DataClient.h"
#include "StringUtils.h"
#include "Xml.h"

namespace Resources {

bool ServicesResource::GetObjects(std::map<std::string, ginger::object>& objects)
{
    if (!TemplateResource::GetObjects(objects))
        return false;

    std::vector<std::map<std::string, ginger::object>> xs;

    auto dataClient = GetSubsystem<IO::DataClient>();
    AB::Entities::ServiceList sl;
    if (!dataClient->Read(sl))
        return false;
    for (const std::string& uuid : sl.uuids)
    {
        AB::Entities::Service s;
        s.uuid = uuid;
        if (!dataClient->Read(s))
            continue;
        xs.push_back({
            { "uuid", s.uuid },
            { "name", Utils::XML::Escape(s.name) },
            { "file", Utils::XML::Escape(s.file) },
            { "host", Utils::XML::Escape(s.host) },
            { "load", std::to_string(s.load) },
            { "location", Utils::XML::Escape(s.location) },
            { "path", Utils::XML::Escape(s.path) },
            { "port", s.port },
            { "run_time", s.runTime },
            { "start_time", s.startTime },
            { "stop_time", s.stopTime },
            { "status", s.status },
            { "online", (bool)(s.status == AB::Entities::ServiceStatusOnline) },
            { "type", s.type },
        });
    }
    objects["services"] = xs;

    return true;
}

ServicesResource::ServicesResource(std::shared_ptr<HttpsServer::Request> request) :
    TemplateResource(request)
{
    template_ = "../templates/services.html";

    footerScripts_.push_back("vendors/gauge.js/dist/gauge.min.js");
}

void ServicesResource::Render(std::shared_ptr<HttpsServer::Response> response)
{
    if (!IsAllowed(AB::Entities::AccountTypeGod))
    {
        Redirect(response, "/");
        return;
    }

    TemplateResource::Render(response);
}

}
