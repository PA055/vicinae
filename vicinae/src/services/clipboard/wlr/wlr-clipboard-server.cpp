#include "wlr-clipboard-server.hpp"
#include <QtConcurrent/qtconcurrentmap.h>
#include <iostream>
#include <ranges>
#include <qapplication.h>
#include <qcontainerfwd.h>
#include <qfuture.h>
#include <qlist.h>
#include <qtimer.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include "services/clipboard/wlr/data-device.hpp"
#include "utils/environment.hpp"

void WlrClipboardServer::global(WaylandRegistry &reg, uint32_t name, const char *interface,
                                uint32_t version) {
  if (strcmp(interface, zwlr_data_control_manager_v1_interface.name) == 0) {
    auto manager = reg.bind<zwlr_data_control_manager_v1>(name, &zwlr_data_control_manager_v1_interface,
                                                          std::min(version, 1u));
    _dcm = std::make_unique<DataControlManager>(manager);
  }

  if (strcmp(interface, wl_seat_interface.name) == 0) {
    _seat = std::make_unique<WaylandSeat>(reg.bind<wl_seat>(name, &wl_seat_interface, std::min(version, 1u)));
  }
}

void WlrClipboardServer::selection(DataDevice &device, DataOffer &offer) {
  auto mimes = offer.mimes();
  std::vector<QFuture<QByteArray>> futures;

  qDebug() << "got selection with" << mimes.size() << "mimes";

  for (const auto &mime : mimes) {
    auto future = offer.receive(mime);

    break;
  }

  /*
  QtFuture::whenAll(futures.begin(), futures.end())
      .then([this, mimes](const QList<QFuture<QByteArray>> &list) {
        ClipboardSelection selection;
        selection.offers.reserve(mimes.size());

        for (const auto &[future, mime] : std::views::zip(list, mimes)) {
          ClipboardDataOffer doffer;

          qDebug() << "result ready" << future.isResultReadyAt(0) << "for mime" << mime;
          doffer.data = future.result();
          doffer.mimeType = QString::fromStdString(mime);
          selection.offers.emplace_back(doffer);
        }

        emit selectionAdded(selection);
      });
          */
}

bool WlrClipboardServer::isAlive() const { return true; }

bool WlrClipboardServer::isActivatable() const {
  return Environment::isWaylandSession() && !Environment::isGnomeEnvironment();
};

QString WlrClipboardServer::id() const { return "wlr-clipboard"; }

int WlrClipboardServer::activationPriority() const { return 1; };

bool WlrClipboardServer::start() {
  auto display = std::make_unique<WaylandDisplay>(
      qApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display());
  _seat =
      std::make_unique<WaylandSeat>(qApp->nativeInterface<QNativeInterface::QWaylandApplication>()->seat());
  _registry = display->registry();

  _registry->addListener(this);
  display->roundtrip();

  if (!_dcm) { throw std::runtime_error("zwlr data control is not available"); }
  if (!_seat) { throw std::runtime_error("seat is not available"); }

  auto dev = _dcm->getDataDevice(*_seat.get()).release();

  dev->registerListener(this);
  display->roundtrip();

  /*
  for (;;) {
    try {
      if (display->dispatch() == -1) { exit(1); }
    } catch (const std::exception &e) { std::cerr << "Uncaught exception: " << e.what() << std::endl; }
  }
  */

  return true;
}

WlrClipboardServer::WlrClipboardServer() : _dcm(nullptr), _seat(nullptr) {}
