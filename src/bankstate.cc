#include "bankstate.h"

namespace dramsim3 {

BankState::BankState()
    : state_(State::CLOSED),
      cmd_timing_(static_cast<int>(CommandType::SIZE)),
      open_row_(-1),
      row_hit_count_(0) {
    cmd_timing_[static_cast<int>(CommandType::READ)] = 0;
    cmd_timing_[static_cast<int>(CommandType::READ_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITE_PRECHARGE)] = 0;

    cmd_timing_[static_cast<int>(CommandType::READCOPY_FPM)] = 0;                   // ---- ROWCLONE ADDED
    cmd_timing_[static_cast<int>(CommandType::READCOPY_PSM)] = 0;
    cmd_timing_[static_cast<int>(CommandType::READCOPY_PSM_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITECOPY_FPM)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITECOPY_FPM_PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITECOPY_PSM)] = 0;
    cmd_timing_[static_cast<int>(CommandType::WRITECOPY_PSM_PRECHARGE)] = 0;        // ---- ROWCLONE ADDED

    cmd_timing_[static_cast<int>(CommandType::ACTIVATE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::PRECHARGE)] = 0;
    cmd_timing_[static_cast<int>(CommandType::REFRESH)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SREF_ENTER)] = 0;
    cmd_timing_[static_cast<int>(CommandType::SREF_EXIT)] = 0;
}


Command BankState::GetReadyCommand(const Command& cmd, uint64_t clk) const {
    CommandType required_type = CommandType::SIZE;
    CommandType copy_type = CommandType::SIZE;
    switch (state_) {
        case State::CLOSED:
            //std::cout << "Bank State : CLOSED" << std::endl;
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY: // Rowclone added
                case CommandType::READCOPY_PRECHARGE:
                    required_type = CommandType::ACTIVATE;
                    break;
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    required_type = CommandType::ACTIVATE;
                    break;
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                    required_type = cmd.cmd_type;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::OPEN:
            //std::cout << "Bank State : OPEN" << std::endl;
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY: // Rowclone added
                case CommandType::READCOPY_PRECHARGE:
                    if (cmd.Row() == open_row_) {
                        required_type = cmd.cmd_type;
                    } else {
                        required_type = CommandType::PRECHARGE;
                    }
                    break;
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    if(cmd.isFPM){
                        if(waiting_command_.addr.row == open_row_){
                            required_type = cmd.cmd_type;
                            //if(cmd.addr.bank == 3){std::cout<<"here writecopy"<<std::endl;}
                        }
                    }
                    else{
                        if(cmd.addr.row != open_row_){
                            required_type = CommandType::PRECHARGE;
                        }
                    }
                    break;
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                    required_type = CommandType::PRECHARGE;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::SREF:
            //std::cout << "Bank State : SREF" << std::endl;
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY: // Rowclone added
                case CommandType::READCOPY_PRECHARGE:
                    required_type = CommandType::SREF_EXIT;
                    break;
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    required_type = CommandType::SREF_EXIT;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::WAIT_WRITECOPY:
            // Rowclone added
            //std::cout << "Bank State : WAIT_WRITECOPY" << std::endl;
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY: 
                case CommandType::READCOPY_PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                    // cannot do anything
                    break;
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    // if this wait is for current command, or not
                    /*if(isRightCommand(cmd)){
                        std::cout<<"yea"<<std::endl;
                        // can start WRITECOPY
                        switch(wait_prev_state_){
                            case State::CLOSED:
                                required_type = CommandType::ACTIVATE;
                                if (clk >= cmd_timing_[static_cast<int>(required_type)]){std::cout<<"can"<<std::endl;}
                                break;
                            case State::OPEN:
                                if (cmd.Row() == open_row_) {
                                    required_type = cmd.cmd_type;
                                } else {
                                    required_type = CommandType::PRECHARGE;
                                }
                                if (clk >= cmd_timing_[static_cast<int>(required_type)]){std::cout<<"can"<<std::endl;}
                                break;
                            case State::SREF:
                                required_type = CommandType::SREF_EXIT;
                                if (clk >= cmd_timing_[static_cast<int>(required_type)]){std::cout<<"can"<<std::endl;}
                                break;
                            default:
                                std::cerr<<"Wrong State!"<<std::endl;
                                AbruptExit(__FILE__, __LINE__);
                                break;
                        }
                        break;
                    } else{
                        // cannot do anything
                        break;
                    }
                    break;*/
                    required_type = cmd.cmd_type;
                    break;
                default:
                    std::cerr << "Unknown type!" << std::endl;
                    AbruptExit(__FILE__, __LINE__);
                    break;
            }
            break;
        case State::PD:
            //std::cout << "Bank State : PD" << std::endl;
        case State::SIZE:
            //std::cerr << "In unknown state" << std::endl;
            AbruptExit(__FILE__, __LINE__);
            break;
    }

    if (required_type != CommandType::SIZE) {
        if((required_type == CommandType::READCOPY) || (required_type == CommandType::WRITECOPY) ||
                (required_type == CommandType::READCOPY_PRECHARGE) ||(required_type == CommandType::WRITECOPY_PRECHARGE)){

            if((required_type == CommandType::READCOPY) && (cmd.isFPM)){copy_type = CommandType::READCOPY_FPM;}
            else if((required_type == CommandType::READCOPY) && (!cmd.isFPM)){copy_type = CommandType::READCOPY_PSM;}
            else if((required_type == CommandType::READCOPY_PRECHARGE) && (cmd.isFPM)){copy_type = CommandType::READCOPY_FPM;}
            else if((required_type == CommandType::READCOPY_PRECHARGE) && (!cmd.isFPM)){copy_type = CommandType::READCOPY_PSM_PRECHARGE;}
            else if((required_type == CommandType::WRITECOPY) && (cmd.isFPM)){copy_type = CommandType::WRITECOPY_FPM;}
            else if((required_type == CommandType::WRITECOPY) && (!cmd.isFPM)){copy_type = CommandType::WRITECOPY_PSM;}
            else if((required_type == CommandType::WRITECOPY_PRECHARGE) && (cmd.isFPM)){copy_type = CommandType::WRITECOPY_FPM_PRECHARGE;}
            else {copy_type = CommandType::WRITECOPY_PSM_PRECHARGE;}

            if (clk >= cmd_timing_[static_cast<int>(copy_type)]) {
                return Command(required_type, cmd.addr, cmd.hex_addr);
            }

        }
        else{
            if (clk >= cmd_timing_[static_cast<int>(required_type)]) {
                /*switch(cmd.cmd_type){
                    case CommandType::READCOPY:
                    case CommandType::READCOPY_PRECHARGE:
                        std::cout<<"READCOPY ";
                        break;
                    case CommandType::WRITECOPY:
                    case CommandType::WRITECOPY_PRECHARGE:
                        std::cout<<"WRITECOPY ";
                        break;
                    default:
                        std::cout<<"ELSE ";
                        break;
                }*/
                return Command(required_type, cmd.addr, cmd.hex_addr);
            }
        }
    }
    return Command();
}

void BankState::UpdateState(const Command& cmd) {
    switch (state_) {
        case State::OPEN:
            switch (cmd.cmd_type) {
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READCOPY:
                case CommandType::READCOPY_PRECHARGE:
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    row_hit_count_++;
                    break;
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                    state_ = State::CLOSED;
                    open_row_ = -1;
                    row_hit_count_ = 0;
                    break;
                case CommandType::ACTIVATE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                case CommandType::SREF_EXIT:
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::CLOSED:
            switch (cmd.cmd_type) {
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                    break;
                case CommandType::ACTIVATE:
                    state_ = State::OPEN;
                    open_row_ = cmd.Row();
                    break;
                case CommandType::SREF_ENTER:
                    state_ = State::SREF;
                    break;
                case CommandType::READCOPY:
                case CommandType::READCOPY_PRECHARGE:
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::PRECHARGE:
                case CommandType::SREF_EXIT:
                default:
                    std::cout << cmd << std::endl;
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::SREF:
            switch (cmd.cmd_type) {
                case CommandType::SREF_EXIT:
                    state_ = State::CLOSED;
                    break;
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY:
                case CommandType::READCOPY_PRECHARGE:
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                case CommandType::ACTIVATE:
                case CommandType::PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        case State::WAIT_WRITECOPY: // only activate, precharge
            switch(cmd.cmd_type){
                case CommandType::ACTIVATE:
                    /*state_ = State::OPEN;
                    open_row_ = cmd.Row();
                    break;*/
                case CommandType::PRECHARGE:
                    /*state_ = State::CLOSED;
                    open_row_ = -1;
                    row_hit_count_ = 0;
                    break;*/
                case CommandType::WRITECOPY:
                case CommandType::WRITECOPY_PRECHARGE:
                    state_ = State::OPEN;
                    row_hit_count_++;
                    break;
                case CommandType::SREF_EXIT:
                case CommandType::READ:
                case CommandType::WRITE:
                case CommandType::READ_PRECHARGE:
                case CommandType::WRITE_PRECHARGE:
                case CommandType::READCOPY:
                case CommandType::READCOPY_PRECHARGE:
                case CommandType::REFRESH:
                case CommandType::REFRESH_BANK:
                case CommandType::SREF_ENTER:
                    break;
                default:
                    AbruptExit(__FILE__, __LINE__);
            }
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return;
}

void BankState::UpdateTiming(CommandType cmd_type, uint64_t time) {
    cmd_timing_[static_cast<int>(cmd_type)] =
        std::max(cmd_timing_[static_cast<int>(cmd_type)], time);
    return;
}

void BankState::StartWaitWriteCopy(const Command& cmd) {
    waiting_command_ = cmd;
    wait_prev_state_ = state_;
    state_ = State::WAIT_WRITECOPY;
}

bool BankState::isRightCommand(const Command& cmd) const{
    return (waiting_command_.hex_addr.src_addr == cmd.hex_addr.src_addr && \
                        waiting_command_.hex_addr.dest_addr == cmd.hex_addr.dest_addr);
}

void BankState::FPMWaitWritecopy(const Command &cmd) {
    waiting_command_ = cmd;
}

bool BankState::CanStartWait(Address dest, uint64_t clk) const {
    if (open_row_ == dest.row && state_ == State::OPEN){
        return true;
    }
    return false;
}

}  // namespace dramsim3
