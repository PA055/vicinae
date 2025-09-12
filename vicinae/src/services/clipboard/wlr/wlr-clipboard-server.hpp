#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <memory>
#include <sys/syscall.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include "data-control-client.hpp"
#include "services/clipboard/clipboard-server.hpp"

class WlrClipboardServer : public AbstractClipboardServer,
                           public WaylandRegistry::Listener,
                           public DataDevice::Listener {

public:
  bool start() override;
  WlrClipboardServer();

  bool isAlive() const override;
  bool isActivatable() const override;
  QString id() const override;
  int activationPriority() const override;

private:
  std::unique_ptr<WaylandRegistry> _registry;
  std::unique_ptr<DataControlManager> _dcm;
  std::unique_ptr<WaylandSeat> _seat;

  void global(WaylandRegistry &reg, uint32_t name, const char *interface, uint32_t version) override;
  void selection(DataDevice &device, DataOffer &offer) override;
};
