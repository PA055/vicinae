#include "data-offer.hpp"
#include <cstring>
#include <fcntl.h>
#include <qapplication.h>
#include <QtConcurrent/QtConcurrent>
#include <qcontainerfwd.h>
#include <qlogging.h>
#include <qsocketnotifier.h>
#include <unistd.h>

DataOffer::DataOffer(zwlr_data_control_offer_v1 *offer) : _offer(offer) {
  zwlr_data_control_offer_v1_add_listener(offer, &_listener, this);
}

const std::vector<std::string> &DataOffer::mimes() const { return _mimes; }

void DataOffer::offer(void *data, zwlr_data_control_offer_v1 *offer, const char *mime) {
  auto self = static_cast<DataOffer *>(data);

  self->_mimes.push_back(mime);
}

QFuture<QByteArray> DataOffer::DataOffer::receive(const std::string &mime) {
  std::string data;
  QPromise<QByteArray> promise;
  auto future = promise.future();

  auto fut = QtConcurrent::run([this, mime]() {
    int pipefd[2];
    if (pipe(pipefd)) { throw std::runtime_error(std::string("Failed to pipe(): ") + strerror(errno)); }

    qDebug() << "offer receive for" << mime;
    zwlr_data_control_offer_v1_receive(_offer, mime.c_str(), pipefd[1]);
    //  Important, otherwise we will block on read forever
    // wl_display_flush(qApp->nativeInterface<QNativeInterface::QWaylandApplication>()->display());
    close(pipefd[1]);
    qDebug() << "offer received for" << mime;

    return 1;
  });

  /*

  auto notifier = new QSocketNotifier(QSocketNotifier::Type::Read);

  // fcntl(pipefd[0], F_SETFD, (fcntl(pipefd[0], F_GETFD) | O_NONBLOCK));

  notifier->setSocket(pipefd[0]);
  notifier->setEnabled(true);
  */

  /*
  QObject::connect(notifier, &QSocketNotifier::activated, [notifier, promise = std::move(promise)]() mutable {
    int rc = 0;
    std::array<char, 8096> buf;
    QByteArray data;

    qDebug() << "reading from pipe...";
    while ((rc = read(notifier->socket(), buf.data(), buf.size())) > 0) {
      qDebug() << "read" << rc << "from pipe";
      data += std::string_view(buf.data(), rc);
    }

    if (rc == -1) { perror("failed to read read end of the pipe"); }

    notifier->deleteLater();
    close(notifier->socket());
    promise.addResult(data);
    promise.finish();
  });
  */

  qDebug() << "returned future for" << mime;

  return future;
}

DataOffer::~DataOffer() {
  qDebug() << "~DataOffer";
  if (_offer) { zwlr_data_control_offer_v1_destroy(_offer); }
}
