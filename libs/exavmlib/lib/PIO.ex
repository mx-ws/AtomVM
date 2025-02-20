#
# This file is part of AtomVM.
#
# Copyright 2018-2020 Davide Bettio <davide@uninstall.it>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0 OR LGPL-2.1-or-later
#

defmodule PIO do
  @compile {:no_warn_undefined, [AVMPort]}
  @moduledoc """
  Functions for interacting with PIO state machines of RP2040-based micro-controllers.
  """

  @doc """
  Set the directional mode of a gpio pin.

  ## Parameters
    - gpio_num:  number of the pin to configure
    - direction: mode :input, :output, or :output_od

  Used to set the direction of a pin before read or write operations.
  See '@type direction()' for more details.
  """
  @spec init(integer()) :: :ok | :error
  def init(some_number),
    do: throw(:nif_error)

end
