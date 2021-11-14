/*
 * Copyright (C) 2021 Chupligin Sergey <neochapay@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include "transaction.h"
#include <QDebug>
#include <QEventLoop>

Transaction::Transaction(QObject *parent) : QObject(parent)
{
    m_database = new DataBase();
    m_pmTransaction = pamac_transaction_new(m_database->db());

    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"emit_action",
                      reinterpret_cast<GCallback>(+([](GObject* obj,char* action){
                          Q_UNUSED(obj);
                          qDebug() << "Action:" << action;
                      })),this);


    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"emit_action_progress",
                     reinterpret_cast<GCallback>(+[](GObject* obj,char* action,char* status,double progress,Transaction* t){
                         Q_UNUSED(obj);

                         t->debug(QString::fromUtf8(action),QString::fromUtf8(status),progress);
                     }),this);

    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"emit_error",
                     reinterpret_cast<GCallback>(+[](GObject* obj,char* message,char** details,int size,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "Error" << message << details;
                         Q_EMIT t->emitError(QString::fromUtf8(message));
                     }),this);

    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"emit_warning",
                     reinterpret_cast<GCallback>(+[](GObject* obj,char* warning,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "Warning" << warning;
                     }),this);
    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"start_preparing",
                     reinterpret_cast<GCallback>(+[](GObject* obj,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "startPreparing";
                     }),this);
    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"stop_preparing",
                     reinterpret_cast<GCallback>(+[](GObject* obj,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "stopPreparing";
                     }),this);

    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"emit_script_output",
                     reinterpret_cast<GCallback>(+[](GObject* obj,char* message,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "emitScriptOutput" << message;
                     }),this);

    g_signal_connect(static_cast<PamacTransaction*>(m_pmTransaction),"important_details_outpout",
                     reinterpret_cast<GCallback>(+[](GObject* obj,bool message,Transaction* t){
                         Q_UNUSED(obj);
                         qDebug() << "importantDetailsOutput" << message;
                     }),this);


}

Transaction::~Transaction(){
    pamac_transaction_quit_daemon(m_pmTransaction);
}

void Transaction::getAuthorization(){
    qDebug() << Q_FUNC_INFO;
    pamac_transaction_get_authorization_async(m_pmTransaction,getAuthorizationFinish,this);
}

void Transaction::install(QStringList packages)
{
    for(int i=0; i < packages.size(); i++) {
        pamac_transaction_add_pkg_to_install(m_pmTransaction, packages.at(i).toUtf8());
    }
    pamac_transaction_run_async(m_pmTransaction,transactionFinish,this);
}

void Transaction::remove(QStringList packages)
{
    for(int i=0; i < packages.size(); i++) {
        pamac_transaction_add_pkg_to_remove(m_pmTransaction, packages.at(i).toUtf8());
    }
    pamac_transaction_run_async(m_pmTransaction,transactionFinish,this);
}

void Transaction::getAuthorizationFinish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    qDebug() << Q_FUNC_INFO;
    Transaction *transaction = static_cast<Transaction*>(user_data);
    Q_EMIT transaction->getAuthorizationReady(pamac_transaction_get_authorization_finish(transaction->m_pmTransaction, res));
}

void Transaction::askCommitFinish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    qDebug() << Q_FUNC_INFO;
}

void Transaction::transactionFinish(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    qDebug() << Q_FUNC_INFO;

    Transaction *transaction = static_cast<Transaction*>(user_data);
    pamac_transaction_quit_daemon(transaction->m_pmTransaction);
    transaction->m_database->refresh();
}

void Transaction::debug(const QString &emitAction, const QString &status, double progress)
{
    qDebug() << emitAction;
    qDebug() << status;
    qDebug() << progress;
}
