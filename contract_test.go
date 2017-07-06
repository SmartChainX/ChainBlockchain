package release



import (

	"crypto/ecdsa"

	"math/big"

	"testing"
  
  // setupReleaseTest creates a blockchain simulator and deploys a version oracle

// contract for testing.

func setupReleaseTest(t *testing.T, prefund ...*ecdsa.PrivateKey) (*ecdsa.PrivateKey, *ReleaseOracle, *backends.SimulatedBackend) {

	// Generate a new random account and a funded simulator

	key, _ := crypto.GenerateKey()

	auth := bind.NewKeyedTransactor(key)



	alloc := core.GenesisAlloc{auth.From: {Balance: big.NewInt(10000000000)}}

	for _, key := range prefund {

		alloc[crypto.PubkeyToAddress(key.PublicKey)] = core.GenesisAccount{Balance: big.NewInt(10000000000)}

	}

	sim := backends.NewSimulatedBackend(alloc)



	// Deploy a version oracle contract, commit and return

	_, _, oracle, err := DeployReleaseOracle(auth, sim, []common.Address{auth.From})

	if err != nil {

		t.Fatalf("Failed to deploy version contract: %v", err)

	}

	sim.Commit()



	return key, oracle, sim

}

