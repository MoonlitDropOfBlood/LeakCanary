import { Database, DatabaseDao, RdbStoreWrapper, SQL } from "@zxhhyj/storm"
import { Context } from "@kit.AbilityKit"
import { relationalStore } from "@kit.ArkData"
import { analysisTaskTable } from "./DatabaseInterfaces"

class AppDatabase extends Database {
  readonly analysisTaskDao = DatabaseDao.form(this).select(analysisTaskTable)

  protected initDb(context: Context) {
    return relationalStore.getRdbStore(context, { name: 'app.db', securityLevel: relationalStore.SecurityLevel.S1 })
  }

  protected onCreate(_rdbStore: RdbStoreWrapper): void {
  }

  protected onDatabaseCreate(rdbStore: RdbStoreWrapper): void {
    rdbStore.executeSync(SQL.createTableAndIndex(analysisTaskTable))
  }
}

export const appDatabase = new AppDatabase(1)