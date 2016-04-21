#include "../__dep__.h"
#include "../command.h"
#include "sched.h"
#include "exec.h"
#include "coord.h"

namespace rococo {

int TapirSched::OnDispatch(const vector<SimpleCommand> &cmd,
                           int32_t* res,
                           TxnOutput *output,
                           const function<void()> &callback) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
//  auto exec = (TapirExecutor*) GetOrCreateExecutor(cmd.root_id_);
  auto exec = GetOrCreateExecutor(cmd[0].root_id_);
//  verify(0);
  exec->mdb_txn();
  for (const auto& c: cmd) {
    exec->Execute(c, res, (*output)[c.inn_id()]);
  }

  callback();
  return 0;

}

int TapirSched::OnFastAccept(cmdid_t cmd_id,
                             const vector<SimpleCommand>& txn_cmds,
                             int32_t* res,
                             const function<void()>& callback) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  Log_debug("receive fast accept for cmd_id: %llx", cmd_id);
  auto exec = (TapirExecutor*) GetOrCreateExecutor(cmd_id);
  exec->FastAccept(txn_cmds, res);

  // DEBUG
  verify(txn_cmds.size() > 0);
  for (auto& c: txn_cmds) {

  }
  callback();
  return 0;
}

int TapirSched::OnDecide(cmdid_t cmd_id,
                         int32_t decision,
                         const function<void()>& callback) {
  std::lock_guard<std::recursive_mutex> lock(mtx_);
  auto exec = (TapirExecutor*) GetExecutor(cmd_id);
  verify(exec);
  if (decision == TapirCoord::Decision::COMMIT) {
    exec->Commit();
  } else if (decision == TapirCoord::Decision::ABORT) {
    exec->Abort();
  } else {
    verify(0);
  }
  DestroyExecutor(cmd_id);
  callback();
}

} // namespace rococo