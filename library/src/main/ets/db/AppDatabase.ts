import { Database, DatabaseDao, Migration, RdbStoreWrapper, SQL } from "@zxhhyj/storm"
import { Context } from "@kit.AbilityKit"
import { relationalStore } from "@kit.ArkData"
import { analysisTaskTable } from "./DatabaseInterfaces"

class AppDatabase extends Database {
  readonly analysisTaskDao = DatabaseDao.form(this).select(analysisTaskTable)
  context:Context
  protected initDb(context: Context) {
    this.context = context
    return relationalStore.getRdbStore(context, { name: 'app.db', securityLevel: relationalStore.SecurityLevel.S1 })
  }

  protected onCreate(_rdbStore: RdbStoreWrapper): void {
  }

  protected onDatabaseCreate(rdbStore: RdbStoreWrapper): void {
    rdbStore.executeSync(SQL.createTableAndIndex(analysisTaskTable))
  }

  /**
   * 定义迁移函数，并使用 @Migration 注解，参数分别为起始版本号和结束版本号
   */
  @Migration(1,2)
  protected migration_1_2(rdbStore: RdbStoreWrapper) {
    // 执行迁移逻辑
    rdbStore.executeSync(SQL.alterTable(analysisTaskTable, sql => sql.addColumn(analysisTaskTable.hashFile)))
  }
}

export const appDatabase = new AppDatabase(2)