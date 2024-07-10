#pragma once

#include "MessageID.hpp"

#include "platform/BaseTask.hpp"

#include "platform/io/BaseMessageReader.hpp"
#include "platform/io/BaseMessageWriter.hpp"
#include "platform/io/BufferMessagePair.hpp"
#include "platform/io/MessageListener.hpp"

#include "platform/hardware/PeripheralConnection.hpp"

#include "control/Command.hpp"
#include "control/DataSummary.hpp"

#include "TaskHubTelemetry.hpp"

class TaskHub : public BaseTask {
    public:
        TaskHub(BaseMessageReader& reader, BaseMessageWriter& writer)
            : reader(reader), writer(writer) {}

        bool setup() override;
        void loop() override;

    protected:
        void process_message(const uint16_t mid);
        void set_rgb(const float r, const float g, const float b);

    protected:
        // PeripheralConnection peripherals[4];

        BaseMessageReader& reader;
        BaseMessageWriter& writer;
        MessageListener<1, 200> ml;

        // Messages handled.
        BufferMessagePair<TaskHubTelemetry> tm;
        BufferMessagePair<DataSummary> tm_data_summary;
        BufferMessagePair<Command> cmd;  // <->
};
